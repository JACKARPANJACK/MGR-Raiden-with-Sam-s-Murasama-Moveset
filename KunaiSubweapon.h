#pragma once
#include "KunaiPolicy.h"
#include "SamMovesetManager.h"
#include "SamElectricCombat.h"
#include "NativeKnifeInventory.h"
#include <PlayerManagerImplement.h>
#include <atomic>
#include <array>
#include <vector>
#include <memory>
#include <deque>
#include <cCameraGame.h>

// Inventory-backed projectiles release independently of the melee graph.
// Camera aiming and charge never cast Raiden to the larger DLC wolf controller.
class KunaiSubweapon
{
    using Descriptor = std::array<unsigned char,0x320>;
    struct Shot
    {
        EntityHandle projectile;
        int variant=0;
        unsigned age=0, fade=0;
        bool spent=false;
        Hw::cVec4 position;
        Descriptor grenade{};
        cEspControler visual;
        EntityHandle target;
        bool homing=false;
        unsigned homingDelay=0;
        ~Shot()
        {
            if (Entity* live=projectile.getEntity()) live->release();
            // SDK's empty virtual destructor does not unlink native particles.
            reinterpret_cast<void(__thiscall*)(cEspControler*)>(shared::base+0xAAA9B0)(&visual);
        }
    };
    struct Burn { EntityHandle target; unsigned remaining=90, interval=0; };
    struct BeforeHit { EntityHandle target; int hp=0; };
    std::vector<std::unique_ptr<Shot>> shots;
    std::vector<Burn> burns;
    std::vector<BeforeHit> beforeHits;
    SamElectricCombat electric;
    std::atomic<int> selected{0}, pending{-1};
    Pl0000* owner=nullptr;
    int previousSlot=0;
    bool wasPrevious=false, wasNext=false;
    
    bool installed=false;
    bool throwReserved=false;
    unsigned throwMask=0, throwOn=0, throwTrig=0, throwRep=0;
    KunaiPolicy::Burst burst;
    KunaiPolicy::ThrowPose pose;
    bool aimLatched=false, poseFrozen=false;
    bool poseIsAim=false;
    unsigned throwKeyOn=0, throwKeyTrig=0, throwKeyRep=0;
    KunaiPolicy::Charge charge;
    struct ReleaseRequest
    {
        unsigned frames=0;
        int variant=KunaiPolicy::Heatblades;
        KunaiPolicy::Vector direction;
        Hw::cVec4 point;
        std::array<EntityHandle,3> targets;
        bool precision=false;
    };
    std::deque<ReleaseRequest> releases;
    ReleaseRequest heldAim;
    bool heldAimValid=false;
    std::atomic<unsigned> aimFrames{0},aimTargets{0},aimAmmo{0},aimRecovery{0};
    std::atomic<bool> aimVisible{false};
    static constexpr unsigned ThrowKey=1u<<('C'%32);

    static bool Blocked()
    { return Trigger::StpFlags.STP_OBJ || g_StaFlags.STA_QTE || g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT; }
    static unsigned Handle(const EntityHandle& h)
    { unsigned raw=0; static_assert(sizeof(h)==sizeof(raw)); std::memcpy(&raw,&h,sizeof(raw)); return raw; }
    static float DistanceSquared(const Hw::cVec4& a,const Hw::cVec4& b)
    { const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z; }
    static Behavior* NativeCreate(Entity* actor,void* descriptor)
    { return reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(actor,descriptor); }
    static bool ReticleRay(Hw::cVec4& origin,KunaiPolicy::Vector& direction)
    {
        // Use the same viewport and unprojection as native worldToScreen;
        // mouse aim, camera pitch/roll, FOV and aspect ratio remain authoritative.
        const int width=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98A90)();
        const int height=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98AA0)();
        if(width<=0 || height<=0) return false;
        Hw::cVec4 world(0,0,0,1);
        const Hw::cVec4 screen(width*KunaiPolicy::ReticleX,height*KunaiPolicy::ReticleY,0.5f,1);
        g_GameCamera.screenToWorld(world,screen);
        origin=g_GameCamera.m_Trans;
        const KunaiPolicy::Vector offset{world.x-origin.x,world.y-origin.y,world.z-origin.z};
        const float length=KunaiPolicy::Length(offset);
        if(!std::isfinite(length) || length<0.0001f) return false;
        direction=KunaiPolicy::Unit(offset);
        return true;
    }
    void AnimateThrow(Pl0000* player,bool heldAim=false)
    {
        if(!KunaiPolicy::CanStartPose(pose.remaining!=0,poseIsAim,heldAim)) return;
        if(!KunaiPolicy::CanAnimateThrow(player->m_Rno0,player->isInAir()!=FALSE,
            player->isBladeModeActive()!=FALSE)) return;
        if(player->m_CurrentInput.m_On & (player->m_ButtonLightAttack|player->m_ButtonHeavyAttack|
            player->m_ButtonJump|player->m_ButtonNinjarun|player->m_ButtonBlademode)) return;
        const auto clip=SamResourceManager::Instance().GetRaidenClip(heldAim ? "2561" : "2566");
        const unsigned length=SamStagePolicy::Length(clip.motion);
        if(!length) return;
        const int returnMap=4; // rest, unless a native movement/attack node takes over
        // Held kunai aim uses Raiden's 2561 motion, without the sequence's
        // grenade state flags or another projectile/ammo event. This does not
        // enter the native grenade controller or borrow Wolf's larger object.
        if(poseFrozen && player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray &&
            player->m_pAnimationSlot->m_pArray[0].m_SlotId==pose.node)
        {
            player->setNodePlaybackSpeed(pose.node,KunaiPolicy::AimPlayback);
            poseFrozen=false;
        }
        const int throwNode=player->setDirectAnimation(clip.motion,nullptr,0,
            heldAim ? KunaiPolicy::ThrowBlend : KunaiPolicy::ReleaseBlend,1.0f,
            0x8000000,0.0f,heldAim ? KunaiPolicy::AimPlayback : KunaiPolicy::ThrowPlayback);
        if(throwNode==-1) return;
        pose={};pose.Begin(KunaiPolicy::PoseTicks(length,heldAim),throwNode,returnMap);
        poseIsAim=heldAim;
        poseFrozen=false;
    }
    void TickThrowPose(Pl0000* player,bool holdAim,bool pendingThrow)
    {
        if(!pose.remaining) return;
        const unsigned interrupt=player->m_ButtonLightAttack|player->m_ButtonHeavyAttack|
            player->m_ButtonJump|player->m_ButtonNinjarun|player->m_ButtonBlademode;
        const bool neutral=KunaiPolicy::CanAnimateThrow(player->m_Rno0,player->isInAir()!=FALSE,
            player->isBladeModeActive()!=FALSE) && !(player->m_CurrentInput.m_On&interrupt);
        const bool owns=player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray &&
            player->m_pAnimationSlot->m_pArray[0].m_SlotId==pose.node &&
            !std::strcmp(player->m_pAnimationSlot->m_pArray[0].m_pAnimName,"Direct");
        // Returning through the previous native map preserves its normal blend;
        // replaced movement/attack nodes own their own completion.
        const bool retainAim=poseIsAim && (holdAim || pendingThrow);
        if(!Blocked() && owns && neutral && retainAim && pose.remaining<=2)
        {
            if(!poseFrozen) player->setNodePlaybackSpeed(pose.node,0);
            poseFrozen=true;return;
        }
        if(owns && poseFrozen && (!retainAim || !neutral))
        {
            player->setNodePlaybackSpeed(pose.node,poseIsAim ? KunaiPolicy::AimPlayback : KunaiPolicy::ThrowPlayback);
            poseFrozen=false;
        }
        if(poseIsAim && !holdAim && !pendingThrow && !Blocked())
        {
            if(owns && neutral) player->requestAnimationByMap(pose.returnMap);
            pose={};poseFrozen=poseIsAim=false;return;
        }
        if(pose.Tick(!Blocked(),owns,neutral)) player->requestAnimationByMap(pose.returnMap);
        if(!pose.remaining) poseFrozen=false;
    }
    static std::array<Entity*,3> AcquireTargets(const Hw::cVec4& origin,KunaiPolicy::Vector direction,float cone)
    {
        std::array<Entity*,3> targets{};
        std::array<float,3> scores{1e9f,1e9f,1e9f};
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        const unsigned count=unsigned((std::max)(0,(std::min)(8192,list.getSize())));
        for(unsigned i=0;i<count && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
        {
            Entity* candidate=*it;if(!SamTargets::Enemy(candidate)) continue;
            const auto p=candidate->getTransPos();
            const float score=KunaiPolicy::AimScore({p.x-origin.x,p.y+0.8f-origin.y,p.z-origin.z},direction,60,cone);
            if(score<0) continue;
            for(unsigned slot=0;slot<targets.size();++slot) if(score<scores[slot])
            {
                for(unsigned j=unsigned(targets.size())-1;j>slot;--j)
                {targets[j]=targets[j-1];scores[j]=scores[j-1];}
                targets[slot]=candidate;scores[slot]=score;break;
            }
        }
        return targets;
    }
    void GuideShots(Pl0000* player)
    {
        if(Blocked() || !player->isAlive()) return;
        for(auto& shot:shots)
        {
            if(!shot->homing || shot->spent || shot->fade || shot->age<shot->homingDelay ||
                shot->age>=KunaiPolicy::HomingLifetime) continue;
            Entity* bullet=shot->projectile.getEntity();
            if(!bullet || !bullet->m_pBehavior || unsigned(bullet->m_ObjId)!=KunaiPolicy::Object) continue;
            auto* velocity=reinterpret_cast<Hw::cVec4*>(reinterpret_cast<unsigned char*>(bullet->m_pBehavior)+0x920);
            const KunaiPolicy::Vector current{velocity->x,velocity->y,velocity->z};
            const auto position=bullet->getTransPos();
            Entity* target=shot->target.getEntity();
            if(!SamTargets::Enemy(target) || DistanceSquared(position,target->getTransPos())>3600)
            {
                target=AcquireTargets(position,KunaiPolicy::Unit(current),0.6f)[0];
                shot->target=target;
            }
            if(!target) continue;
            const auto p=target->getTransPos();
            const auto steered=KunaiPolicy::Steer(current,{p.x-position.x,p.y+0.8f-position.y,p.z-position.z});
            // Native BulletBase velocity, verified at 6D054B and 6D03BE.
            // Native physics still advances the projectile and checks walls/hits.
            velocity->x=steered.x;velocity->y=steered.y;velocity->z=steered.z;
        }
    }
    void Detonate(Shot& shot)
    {
        if (shot.spent || !owner || !owner->m_pEntity || !owner->isAlive() || Blocked()) return;
        if(!SamResourceManager::Instance().ExplosivesReady()) return;
        // A real native grenade payload supplies explosion collision, sound and
        // particles. Creating the payload does not consume a second inventory item.
        Descriptor payload=shot.grenade;
        *reinterpret_cast<unsigned*>(payload.data()+4)=KunaiPolicy::GrenadeObject;
        *reinterpret_cast<unsigned*>(payload.data()+0x110)=0x26;
        *reinterpret_cast<unsigned*>(payload.data()+0x114)=0x11;
        auto* hit=reinterpret_cast<CollisionAttackData::HitData*>(payload.data()+0x10);
        hit->field_0=0x57; hit->field_4=0; hit->field_8=24; hit->field_C=20;
        Hw::cVec4 origin=shot.position, target=origin, rotation(0,0,0,0);
        target.y-=0.1f;
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float)>(shared::base+0x16E30)
            (payload.data(),&origin,&target,&rotation,0.01f,120.0f);
        *reinterpret_cast<unsigned*>(payload.data()+0x174)=0; // no stale target attachment
        if(NativeCreate(owner->m_pEntity,payload.data())) shot.spent=true;
    }
    void Impact(Pl0000* player, Entity* target, Shot& shot)
    {
        if (shot.spent) return;
        shot.position=target->getTransPos();
        if (shot.variant==KunaiPolicy::Explosive) { Detonate(shot); return; }
        shot.spent=true;
        if (shot.variant==KunaiPolicy::Stun) electric.Stun(player,target,90);
        else if (shot.variant==KunaiPolicy::Heat || shot.variant==KunaiPolicy::Heatblades)
        {
            bool present=false;
            for (auto& burn : burns) if (burn.target.getEntity()==target) {burn.remaining=90; present=true; break;}
            if (!present && burns.size()<KunaiPolicy::MaxShots) { Burn burn; burn.target=target; burns.push_back(burn); }
            // The native wp0372 bank remains responsible for the heat-blade
            // trail/impact; no Sam-only effect ID is applied to this projectile.
        }
    }
public:
    void AimView(gui::KunaiAimView& out) const
    { out.active=aimVisible.load();out.frames=aimFrames.load();out.targets=aimTargets.load();out.ammo=aimAmmo.load();out.recovery=aimRecovery.load();out.variant=Selected(); }
    static KunaiSubweapon& Get() { static KunaiSubweapon instance; return instance; }
    bool Install()
    {
        const auto call=shared::base+0x7A4883;
        int displacement=0;std::memcpy(&displacement,reinterpret_cast<void*>(call+1),4);
        if(*reinterpret_cast<unsigned char*>(call)!=0xE8 || call+5+displacement!=shared::base+0x6D3BE0) return false;
        installed=true;return true;
    }
    int Selected() const { return selected.load(); }
    int Pending() const { return pending.load(); }
    void RestoreInputs()
    {
        if (!throwReserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~throwMask)|throwOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~throwMask)|throwTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~throwMask)|throwRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~ThrowKey)|throwKeyOn;
        g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~ThrowKey)|throwKeyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~ThrowKey)|throwKeyRep;
        throwReserved=false;
    }
    bool ThrowHeatblade(Pl0000* player,int variant,Entity* enemy,const Hw::cVec4& aimPoint,
        KunaiPolicy::Vector direction,bool homing,bool precision,float spread)
    {
        auto* inventory=NativeKnifeInventory::Item();
        if (!inventory || inventory->noUse() || !g_pPlayerManager ||
            g_pPlayerManager->getSubWeaponEquipped()!=NativeKnifeInventory::Slot ||
            !SamResourceManager::Instance().HeatbladesReady() || shots.size()>=KunaiPolicy::MaxShots ||
            !EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,
                player->isInAir()!=FALSE,Blocked(),player->isBladeModeActive()!=FALSE)) return false;
        alignas(16) Descriptor descriptor{};
        using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(descriptor.data()+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(descriptor.data()+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(descriptor.data());
        auto* data=descriptor.data();
        KunaiPolicy::Configure(data,variant);
        // Wolf's projectile/trajectory with Raiden's friendly hit filtering.
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10000000;
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x8800;
        *reinterpret_cast<unsigned short*>(data+0x20)=0xA00;
        *reinterpret_cast<Entity**>(data+0x24)=player->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=player->m_pEntity;
        auto* category=reinterpret_cast<unsigned*(__thiscall*)(Pl0000*)>(shared::base+0x5F8B60)(player);
        if (!category) return false;
        *reinterpret_cast<unsigned*>(data+0x170)=*category;
        const auto pos=player->m_pEntity->getTransPos();
        const float dx=direction.x,dz=direction.z;
        Hw::cVec4 origin(pos.x+dx*0.65f,pos.y+1.1f,pos.z+dz*0.65f,1);
        Hw::cVec4 target=aimPoint;
        if(!precision && SamTargets::Enemy(enemy)) {target=enemy->getTransPos();target.y+=0.8f;}
        // Aim convergence comes from the raised screen ray, never a chest snap.
        // Charged homing may steer to its weak target after the native launch.
        const float tx=target.x-origin.x,ty=target.y-origin.y,tz=target.z-origin.z;
        Hw::cVec4 rotation(-std::atan2(ty,std::sqrt(tx*tx+tz*tz)),std::atan2(tx,tz)+spread,0,0);
        using Trajectory=void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float);
        reinterpret_cast<Trajectory>(shared::base+0x16E30)(data,&origin,&target,&rotation,0.38f,250.0f);
        Behavior* projectile=NativeCreate(player->m_pEntity,data);
        if (!projectile || !projectile->m_pEntity) return false;
        auto* bytes=reinterpret_cast<unsigned char*>(projectile);
        *reinterpret_cast<int*>(bytes+0xBF4)=0;
        *reinterpret_cast<float*>(bytes+0xBF8)=0.5f;
        *reinterpret_cast<float*>(bytes+0xBFC)=0.3f;
        auto shot=std::make_unique<Shot>();shot->projectile=projectile->m_pEntity;
        shot->position=origin;shot->variant=variant;shot->grenade=descriptor;
        shot->target=enemy;shot->homing=homing;
        shot->homingDelay=spread!=0 ? KunaiPolicy::FanHomingDelay : 0;
        if(variant==KunaiPolicy::Stun) SamElectricCombat::Lightning(player,projectile->m_pEntity,&shot->visual);
        shots.push_back(std::move(shot));
        inventory->use(); // Native ammo flags/count, only after successful spawn.
        return true;
    }
    void Select(int variant) { if(installed && variant>=0 && variant<KunaiPolicy::Count) pending=variant; }
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        RestoreInputs();
        electric.Reset(); burns.clear(); beforeHits.clear();
        SamMovesetManager::Instance().SetSubweaponActive(false);
        shots.clear();
        selected=0; pending=-1; owner=nullptr;burst={};pose={};aimLatched=poseFrozen=poseIsAim=false;
        charge.Cancel(false);releases.clear();heldAimValid=false;aimVisible=false;aimFrames=aimTargets=aimAmmo=aimRecovery=0;
        NativeKnifeInventory::raidenScene=false;
    }
    void Tick(Pl0000* player)
    {
        if(owner!=player) {if(owner) Forget(owner); owner=player;}
        if(!player || !player->m_pEntity || !g_pPlayerManager) return;
        if(!SamNativeRuntime::Get().Owns(player)) { Forget(player);return; }
        NativeKnifeInventory::raidenScene=SamNativeRuntime::Get().Owns(player);
        NativeKnifeInventory::Ensure();
        beforeHits.clear();
        GuideShots(player);
        if(Selected()!=0 || !shots.empty())
        {
            auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
            const unsigned count=unsigned((std::max)(0,(std::min)(8192,list.getSize())));
            for(unsigned visited=0;visited<count && it!=Hw::cFixedList<Entity*>::iterator{};++visited,++it)
            {
                Entity* target=*it;if(!SamTargets::Enemy(target)) continue;
                BeforeHit entry;entry.target=target;entry.hp=static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp;
                beforeHits.push_back(entry);
            }
        }
        DWORD foreground=0; GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool previous=(GetAsyncKeyState(VK_F7)&0x8000)!=0, next=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
        if(foreground==GetCurrentProcessId() && !Blocked() && player->isAlive() && previous!=next)
        {
            if(previous&&!wasPrevious) Select(KunaiPolicy::Cycle(Pending()<0?Selected():Pending(),-1));
            if(next&&!wasNext) Select(KunaiPolicy::Cycle(Pending()<0?Selected():Pending(),1));
        }
        wasPrevious=previous; wasNext=next;
        // Every kunai payload uses the genuine knife item, including menu modes.
        const int slot=g_pPlayerManager->getSubWeaponEquipped();
        if(slot==NativeKnifeInventory::Slot)
        { if(Selected()==KunaiPolicy::Native) selected=KunaiPolicy::Heatblades; }
        else { selected=KunaiPolicy::Native; previousSlot=slot; }
        const int choice=Pending();
        if(choice>=0 && EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,
            player->isInAir()!=FALSE,Blocked(),player->isBladeModeActive()!=FALSE))
        {
            if(choice!=KunaiPolicy::Native)
            {
                if(!NativeKnifeInventory::Ensure() || !SamResourceManager::Instance().HeatbladesReady()) return;
                if(slot!=NativeKnifeInventory::Slot) previousSlot=slot;
                g_pPlayerManager->setSubWeaponEquipped(NativeKnifeInventory::Slot);
            }
            else if(slot==NativeKnifeInventory::Slot)
                g_pPlayerManager->setSubWeaponEquipped(previousSlot==NativeKnifeInventory::Slot ? 0 : previousSlot);
            if(choice!=Selected()) {charge.Cancel(true);releases.clear();heldAimValid=false;aimLatched=false;}
            selected=choice; pending=-1;
        }
        burst.Tick(!Blocked());
        const unsigned button=player->m_ButtonUseSubweapon;
        const bool throwDown=(GetAsyncKeyState('C')&0x8000)!=0 || (g_dbPad.m_On&button)!=0;
        const bool focused=foreground==GetCurrentProcessId() && !gui::IsMenuVisible();
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        const bool eligible=Selected()!=KunaiPolicy::Native && focused &&
            EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,
                player->isInAir()!=FALSE,story,player->isBladeModeActive()!=FALSE);
        if(!eligible) {charge.Cancel(throwDown);releases.clear();heldAimValid=false;aimLatched=false;}
        if(!throwDown) aimLatched=false;
        const bool release=eligible && !Trigger::StpFlags.STP_OBJ && charge.Tick(throwDown,true,true);
        if(eligible && throwDown && KunaiPolicy::PrecisionAim(charge.frames)) aimLatched=true;
        const bool precisionAim=KunaiPolicy::PrecisionAim(charge.frames) || aimLatched;
        Hw::cVec4 cameraPosition=player->m_pEntity->getTransPos();cameraPosition.y+=1.1f;
        KunaiPolicy::Vector direction{std::sin(player->m_Rot.y),0,std::cos(player->m_Rot.y)};
        const bool validAim=!precisionAim || ReticleRay(cameraPosition,direction);
        const auto precision=validAim ? AcquireTargets(cameraPosition,direction,precisionAim?0.995f:0.6f) : std::array<Entity*,3>{};
        float depth=60;
        if(Entity* target=precision[0])
        {
            const auto p=target->getTransPos();
            depth=KunaiPolicy::AimDepth({p.x-cameraPosition.x,p.y+0.8f-cameraPosition.y,p.z-cameraPosition.z},direction);
        }
        Hw::cVec4 aimPoint(cameraPosition.x+direction.x*depth,cameraPosition.y+direction.y*depth,
            cameraPosition.z+direction.z*depth,1);
        if(!precisionAim && precision[0]) {aimPoint=precision[0]->getTransPos();aimPoint.y+=0.8f;}
        const auto locks=validAim && charge.frames>=KunaiPolicy::VolleyCharge ?
            AcquireTargets(cameraPosition,direction,0.94f) : precision;
        auto* item=NativeKnifeInventory::Item();
        const unsigned ammo=item ? unsigned((std::max)(0,item->m_nBasePossession)) : 0;
        aimVisible=throwDown && precisionAim && validAim && eligible;aimFrames=charge.frames;aimAmmo=ammo;
        unsigned targetCount=0;for(Entity* target:locks) if(target) ++targetCount;
        aimTargets=targetCount;
        if(precisionAim && (charge.holding || (release && throwDown)) && eligible && validAim && !Blocked())
        {
            heldAim.precision=true;heldAim.direction=direction;heldAim.point=aimPoint;
            for(unsigned i=0;i<locks.size();++i) heldAim.targets[i]=locks[i];
            heldAimValid=true;
        }
        const bool holdAim=throwDown && aimLatched && eligible && validAim;
        if(release)
        {
            if(ammo && validAim && releases.size()<KunaiPolicy::BurstSize && (!precisionAim || heldAimValid))
            {
                ReleaseRequest request=precisionAim ? heldAim : ReleaseRequest{};
                request.frames=charge.frames;request.variant=Selected();
                if(!precisionAim)
                {
                    request.direction=direction;request.point=aimPoint;
                    for(unsigned i=0;i<locks.size();++i) request.targets[i]=locks[i];
                }
                releases.push_back(request);
            }
            charge.frames=0;aimFrames=0;aimVisible=holdAim;heldAimValid=false;
        }
        TickThrowPose(player,holdAim,!releases.empty());
        if(holdAim && !Blocked()) AnimateThrow(player,true);
        if(!releases.empty() && burst.Capacity() && eligible && !Blocked())
        {
            const auto request=releases.front();releases.pop_front();
            const auto plan=KunaiPolicy::Plan(request.frames,request.variant,ammo,
                (std::min)(burst.Capacity(),unsigned(KunaiPolicy::MaxShots-shots.size())));
            std::array<Entity*,3> targets{};unsigned available=0;
            for(auto& handle:request.targets)
                if(Entity* target=handle.getEntity();SamTargets::Enemy(target)) targets[available++]=target;
            unsigned fired=0;
            if(plan.variant!=KunaiPolicy::Explosive || SamResourceManager::Instance().ExplosivesReady())
                for(unsigned i=0;i<plan.count;++i)
                {
                    Entity* target=available ? targets[i%available] : nullptr;
                    const float spread=request.frames>=KunaiPolicy::VolleyCharge ?
                        KunaiPolicy::FanSpread(i,plan.count) : 0;
                    if(ThrowHeatblade(player,plan.variant,target,request.point,request.direction,plan.homing,request.precision,spread)) ++fired;
                }
            if(fired)
            {
                burst.Fired(fired,item->m_nBasePossession<=0);
                AnimateThrow(player);
                if(item->m_nBasePossession<=0) releases.clear();
            }
        }
        aimRecovery=eligible && burst.recovering ? burst.remaining : 0;
        if(Selected()!=KunaiPolicy::Native && focused && !Blocked() &&
            EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,
                player->isInAir()!=FALSE,false,player->isBladeModeActive()!=FALSE))
        {
            // Keep camera/movement/attack inputs; reserve only knife release.
            throwMask=button&~0xFu;throwOn=g_dbPad.m_On&throwMask;throwTrig=g_dbPad.m_Trig&throwMask;throwRep=g_dbPad.m_Rep&throwMask;
            throwKeyOn=g_Keyboard.m_pOn[2]&ThrowKey;throwKeyTrig=g_Keyboard.m_pTrig[2]&ThrowKey;throwKeyRep=g_Keyboard.m_pRep[2]&ThrowKey;
            g_dbPad.m_On&=~throwMask;g_dbPad.m_Trig&=~throwMask;g_dbPad.m_Rep&=~throwMask;
            g_Keyboard.m_pOn[2]&=~ThrowKey;g_Keyboard.m_pTrig[2]&=~ThrowKey;g_Keyboard.m_pRep[2]&=~ThrowKey;
            player->m_CurrentInput.m_On&=~button;player->m_CurrentInput.m_Trig&=~button;
            throwReserved=true;
        }
    }
    Behavior* Release(Entity* actor,void* original)
    {
        // Native grenades stay native; kunai never enter grenade aim states.
        return NativeCreate(actor,original);
    }
    void PostTick(Pl0000* player)
    {
        if(owner!=player || !player || !player->m_pEntity || Blocked()) return;
        electric.Tick(player,false,false,false,false);
        for(auto& shot:shots) if(Entity* projectile=shot->projectile.getEntity()) shot->position=projectile->getTransPos();
        auto& list=g_EntitySystem.getEntityList(); auto it=list.begin();
        const unsigned count=unsigned((std::max)(0,(std::min)(8192,list.getSize())));
        for(unsigned visited=0;visited<count && it!=Hw::cFixedList<Entity*>::iterator{};++visited,++it)
        {
            Entity* target=*it; if(!SamTargets::Enemy(target)) continue;
            const auto& hits=target->m_pBehavior->m_AttackHits;
            if(!hits.m_vector || hits.m_size>hits.m_capacity || hits.m_size>128) continue;
            bool lostHp=false;
            for(const auto& before:beforeHits) if(before.target.getEntity()==target)
            {lostHp=static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp<before.hp;break;}
            for(size_t h=0;h<hits.m_size;++h) for(auto& shot:shots)
            {
                if(shot->spent) continue;
                const auto& hit=hits.m_vector[h];
                if(!KunaiPolicy::Matches(hit.m_HitData.field_0,hit.m_HitData.field_4,shot->variant,
                    hit.m_HitData.field_18.getEntity()==player->m_pEntity)) continue;
                const unsigned source=Handle(shot->projectile);
                // Collision entity handles identify the shot where available;
                // proximity only disambiguates otherwise identical owned hits.
                const bool identified=source && (Handle(hit.field_120)==source || Handle(hit.field_124)==source);
                if(!identified && (!lostHp || DistanceSquared(shot->position,target->getTransPos())>16.0f)) continue;
                Impact(player,target,*shot);break;
            }
        }
        for(auto& shot:shots)
        {
            Entity* projectile=shot->projectile.getEntity();
            if(projectile) shot->position=projectile->getTransPos();
            ++shot->age;
            if(!shot->spent && KunaiPolicy::TimedDetonation(shot->variant,shot->age,!projectile)) Detonate(*shot);
            if((!projectile || shot->spent || shot->age>=KunaiPolicy::Lifetime) && !shot->fade)
            {shot->visual.FadeUnits(3,0);shot->fade=30;}
        }
        for(auto& burn:burns)
        {
            Entity* target=burn.target.getEntity();
            if(!target || !SamTargets::Enemy(target) || !player->isAlive()) {burn.remaining=0;continue;}
            if(burn.remaining && ++burn.interval>=30)
            {
                burn.interval=0;
                auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);
                enemy->damage(KunaiPolicy::BurnDamage(enemy->m_HpMax),false);
            }
            if(burn.remaining) --burn.remaining;
        }
        burns.erase(std::remove_if(burns.begin(),burns.end(),[](const Burn& burn){return !burn.remaining;}),burns.end());
        shots.erase(std::remove_if(shots.begin(),shots.end(),[](const auto& shot){return shot->fade && --shot->fade==0;}),shots.end());
    }
};
