#pragma once
#include "SmgPolicy.h"
#include "NativeSmgMenuPolicy.h"
#include "SamTargets.h"
#include "SamNativeRuntime.h"
#include "SamStagePolicy.h"
#include "EncounterPolicy.h"
#include "gui.h"
#include "SamVisualEffects.h"
#include "NativeSmgAim.h"
#include <cObjReadManager.h>
#include <array>
#include <vector>
#include <atomic>

// Enemy gun/model and BulletBase factory; never invoke Em0040 AI on Pl0000.
class SmgSubweapon
{
    struct Resource {unsigned id;bool requested=false,pinned=false,bank=false;Hw::cFmerge archive;};
    std::array<Resource,4> resources{{{SmgPolicy::Gun},{SmgPolicy::Bullet},{SmgPolicy::MotionBank},{SmgPolicy::EnemyMotionBank}}};
    struct Shot {EntityHandle handle;unsigned age=0;};
    std::vector<Shot> shots;
    Pl0000* owner=nullptr;
    EntityHandle gun;
    EntityHandle dumpTarget;
    cEspControler muzzleFlash;
    bool flashActive=false;
    SmgPolicy::FireControl fire;
    int poseNode=-1;
    unsigned poseTicks=0;
    bool gunPose=false,readyPose=false,poseFrozen=false;
    SmgPolicy::ReadyWindow readyWindow;
    NativeSmgAim aiming;
    unsigned animatedCombo=0;
    unsigned preparedDumpCombo=0;
    bool pendingShot=false;
    EntityHandle pendingTarget;
    SmgPolicy::Move pendingMove=SmgPolicy::Burst;
    bool reserved=false;
    unsigned mask=0,padOn=0,padTrig=0,padRep=0,keyOn=0,keyTrig=0,keyRep=0;
    static constexpr unsigned Key=1u<<('C'%32), Constraint=14;
    std::atomic<unsigned> displayRounds{SmgPolicy::Magazine},displayReload{0};
    std::atomic<unsigned> displayCharge{0};
    std::atomic<bool> displayReady{false};
    struct Impact {EntityHandle target;unsigned cooldown=0;};
    std::vector<Impact> impacts;
    struct HealthBefore {EntityHandle target;int hp;};
    std::vector<HealthBefore> before;
    static bool Stopped() {return Trigger::StpFlags.STP_OBJ!=0;}
    bool ResourcesReady()
    {
        // Verify the actual enemy SMG descriptor on this executable before use.
        const unsigned char objectBytes[]={0xC7,0x44,0x24,0x74,0x46,0x00,0x02,0x00};
        const unsigned char typeBytes[]={0xC7,0x84,0x24,0x80,0x01,0x00,0x00,0x58,0x00,0x00,0x00};
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x71CEA8),objectBytes,sizeof(objectBytes)) ||
            std::memcmp(reinterpret_cast<void*>(shared::base+0x71CEB0),typeBytes,sizeof(typeBytes))) return false;
        bool ready=true;
        for(auto& r:resources)
        {
            const auto id=static_cast<eObjID>(r.id);
            if(!r.requested) r.requested=g_ObjReadManager.requestObject(id,0)!=FALSE;
            if(!r.requested || !g_ObjReadManager.isObjectLoaded(id,0)) {ready=false;continue;}
            if(!r.pinned) {g_ObjReadManager.addUseRef(id,0);r.pinned=true;}
            const bool needsBank=r.id!=SmgPolicy::Bullet;
            if(needsBank && !r.bank && g_ObjReadManager.getDataAtSet(r.archive,id,0))
                r.bank=reinterpret_cast<BOOL(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00C50)(r.id,&r.archive)!=FALSE;
            ready=(!needsBank || r.bank) && ready;
        }
        return ready;
    }
    void HideGun()
    {
        if(flashActive)
        {
            reinterpret_cast<void(__thiscall*)(cEspControler*)>(shared::base+0xAAA9B0)(&muzzleFlash);
            reinterpret_cast<void(__thiscall*)(cEspControler*)>(shared::base+0xAAA060)(&muzzleFlash);
            flashActive=false;
        }
        Entity* entity=gun.getEntity();
        if(owner && entity && owner->getConstraintsEntity(Constraint)==entity) owner->removeConstraint(Constraint);
        if(entity) entity->release();
        gun=EntityHandle{};
    }
    bool Equip()
    {
        if(gun.getEntity()) return true;
        // Do not steal a constraint used by another plugin/native controller.
        if(owner->getConstraintsEntity(Constraint)) return false;
        Entity* entity=g_EntitySystem.createEntity("Emc040_Smg",static_cast<eObjID>(SmgPolicy::Gun),nullptr);
        if(!entity || !entity->m_pBehavior) {if(entity) entity->release();return false;}
        gun=entity;entity->m_pBehavior->m_pOwner=owner;
        auto initial=owner->m_pEntity->getTransPos();initial.y+=1.1f;
        entity->setTransPos(initial);entity->setRot(owner->m_Rot);
        // Native SMG mounts on right-hand marker 701 (7060B2).
        owner->attachObject(Constraint,owner->m_pEntity,entity,0x701,-1);
        entity->m_pBehavior->onDisp();
        return true;
    }
    bool OwnsPose() const
    {
        return owner && poseTicks && owner->m_pAnimationSlot && owner->m_pAnimationSlot->m_pArray &&
            owner->m_pAnimationSlot->m_pArray[0].m_SlotId==poseNode &&
            !std::strcmp(owner->m_pAnimationSlot->m_pArray[0].m_pAnimName,"Direct");
    }
    void EndPose()
    {
        if(OwnsPose() && owner->isAlive() && !owner->isInAir() &&
            !EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT)) owner->requestAnimationByMap(4);
        aiming.Reset();
        poseTicks=0;poseNode=-1;gunPose=readyPose=poseFrozen=false;
    }
    bool Neutral() const
    {
        const unsigned interrupt=owner->m_ButtonLightAttack|owner->m_ButtonHeavyAttack|
            owner->m_ButtonJump|owner->m_ButtonNinjarun|owner->m_ButtonBlademode;
        return !owner->isBladeModeActive() && !owner->isInAir() &&
            (owner->m_Rno0<=4 || owner->m_Rno0==0x100000) &&
            !(owner->m_CurrentInput.m_On&interrupt) &&
            std::abs(owner->m_TransSpeed.x)+std::abs(owner->m_TransSpeed.z)<0.003f &&
            !(GetAsyncKeyState('W')&0x8000 || GetAsyncKeyState('A')&0x8000 ||
              GetAsyncKeyState('S')&0x8000 || GetAsyncKeyState('D')&0x8000);
    }
    void AimAt(Entity* target)
    {
        if(!gunPose || !OwnsPose()) return;
        auto aim=target?target->getTransPos():owner->m_pEntity->getTransPos();
        if(target)
        {
            aim.y+=0.8f;
            if(Neutral())
            {
                const auto position=owner->m_pEntity->getTransPos();
                const float dx=aim.x-position.x,dz=aim.z-position.z;
                if(dx*dx+dz*dz>0.01f)
                {owner->m_Rot.y=std::atan2(dx,dz);owner->m_pEntity->setRot(owner->m_Rot);}
            }
        }
        else {aim.x+=std::sin(owner->m_Rot.y)*SmgPolicy::Range;aim.z+=std::cos(owner->m_Rot.y)*SmgPolicy::Range;aim.y+=1.1f;}
        aiming.Aim(aim);
    }
    void Animate(bool firingRecoil=false,bool ready=false,bool raise=false)
    {
        if(owner->isBladeModeActive() || owner->isInAir() ||
            !(owner->m_Rno0<=4 || owner->m_Rno0==0x100000)) return;
        // Motion only: these Raiden clips contain scripted RPG callbacks in their sequences.
        // Let the independent projectile controller own every shot.
        aiming.Reset();
        if(poseFrozen && OwnsPose()) owner->setNodePlaybackSpeed(poseNode,1);
        poseFrozen=false;
        const bool special=fire.comboPose && !firingRecoil && !ready && !raise;
        if(!special && !Neutral()) return;
        const char* code=special?fire.AnimationCode():raise?SmgPolicy::RaiseMotion:ready?SmgPolicy::ReadyMotion:"2400";
        char name[40];std::snprintf(name,sizeof(name),"%s_%s.mot",special?"pl0010":"em0040",code);
        void* motion=resources[special?2:3].archive.getFileNameData(name);
        const unsigned length=SamStagePolicy::Length(motion);if(!length) return;
        const float speed=special?2.8f:raise?2.0f:1.0f;
        poseNode=owner->setDirectAnimation(motion,nullptr,0,0.06f,1,0x8000000,0,speed);
        if(poseNode>=0)
        {
            poseTicks=unsigned(std::ceil(length/speed));gunPose=!special;readyPose=ready;
            if(gunPose) aiming.Bind(owner);
            if(special) fire.delay=unsigned(std::ceil(SmgPolicy::MotionRelease(code)/speed));
        }
    }
    bool Spawn(Entity* target,SmgPolicy::Move move)
    {
        Entity* model=gun.getEntity();
        if(!model || !model->m_pBehavior || shots.size()>=SmgPolicy::MaxShots) return false;
        cParts* muzzle=model->m_pBehavior->getPartsPtr(SmgPolicy::Muzzle);
        if(!muzzle) return false; // Never silently fall back to firing from the player/root.
        alignas(16) std::array<unsigned char,0x320> descriptor{};
        auto* data=descriptor.data();using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(data+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(data+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(data);
        SmgPolicy::Configure(data,move);
        *reinterpret_cast<Entity**>(data+0x24)=owner->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=owner->m_pEntity;
        *reinterpret_cast<unsigned short*>(data+0x20)=0xA00;
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10000000;
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x8800;
        auto* category=reinterpret_cast<unsigned*(__thiscall*)(Pl0000*)>(shared::base+0x5F8B60)(owner);
        if(!category) return false;
        *reinterpret_cast<unsigned*>(data+0x170)=*category;
        const float yaw=owner->m_Rot.y;
        Hw::cVec4 origin=muzzle->getPos();origin.w=1;
        Hw::cVec4 aim(origin.x+std::sin(yaw)*SmgPolicy::Range,origin.y,origin.z+std::cos(yaw)*SmgPolicy::Range,1);
        if(SamTargets::Enemy(target)) {aim=target->getTransPos();aim.y+=0.8f;}
        const float dx=aim.x-origin.x,dy=aim.y-origin.y,dz=aim.z-origin.z;
        Hw::cVec4 rot(-std::atan2(dy,std::sqrt(dx*dx+dz*dz)),std::atan2(dx,dz),0,0);
        using Trajectory=void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float);
        reinterpret_cast<Trajectory>(shared::base+0x16E30)(data,&origin,&aim,&rot,1.0f,250.0f);
        Behavior* bullet=reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(owner->m_pEntity,data);
        if(!bullet || !bullet->m_pEntity) return false;
        if(unsigned(bullet->m_pEntity->m_ObjId)!=SmgPolicy::Bullet)
        {bullet->m_pEntity->release();return false;}
        SamVisualEffects::Spawn(model->m_pBehavior,model,SmgPolicy::Gun,SmgPolicy::MuzzleEffect,&muzzleFlash);
        flashActive=true;
        Shot shot;shot.handle=bullet->m_pEntity;shots.push_back(shot);return true;
    }
    Entity* Acquire(bool sweep)
    {
        std::array<Entity*,3> targets{};std::array<float,3> distances{1e9f,1e9f,1e9f};
        const auto pos=owner->m_pEntity->getTransPos();
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
        {
            Entity* candidate=*it;if(!SamTargets::Enemy(candidate) || static_cast<BehaviorAppBase*>(candidate->m_pBehavior)->m_Hp<=0) continue;
            const auto p=candidate->getTransPos();float dx=p.x-pos.x,dz=p.z-pos.z,dy=p.y-pos.y;
            const float distance=dx*dx+dy*dy+dz*dz;
            if(distance>SmgPolicy::Range*SmgPolicy::Range || dx*std::sin(owner->m_Rot.y)+dz*std::cos(owner->m_Rot.y)<-0.5f) continue;
            for(unsigned j=0;j<targets.size();++j) if(distance<distances[j])
            {
                for(unsigned k=unsigned(targets.size())-1;k>j;--k) {targets[k]=targets[k-1];distances[k]=distances[k-1];}
                targets[j]=candidate;distances[j]=distance;break;
            }
        }
        unsigned count=0;while(count<targets.size() && targets[count]) ++count;
        return count ? targets[sweep?fire.ordinal%count:0] : nullptr;
    }
    void Reserve()
    {
        mask=owner->m_ButtonUseSubweapon&~0xFu;
        padOn=g_dbPad.m_On&mask;padTrig=g_dbPad.m_Trig&mask;padRep=g_dbPad.m_Rep&mask;
        keyOn=g_Keyboard.m_pOn[2]&Key;keyTrig=g_Keyboard.m_pTrig[2]&Key;keyRep=g_Keyboard.m_pRep[2]&Key;
        g_dbPad.m_On&=~mask;g_dbPad.m_Trig&=~mask;g_dbPad.m_Rep&=~mask;
        g_Keyboard.m_pOn[2]&=~Key;g_Keyboard.m_pTrig[2]&=~Key;g_Keyboard.m_pRep[2]&=~Key;
        owner->m_CurrentInput.m_On&=~owner->m_ButtonUseSubweapon;
        owner->m_CurrentInput.m_Trig&=~owner->m_ButtonUseSubweapon;reserved=true;
    }
public:
    void View(gui::SmgView& out) const
    {out.rounds=displayRounds.load();out.reload=displayReload.load();out.charge=displayCharge.load();out.ready=displayReady.load();}
    void RestoreInputs()
    {
        if(!reserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|padOn;g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|padTrig;g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|padRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~Key)|keyOn;g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~Key)|keyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~Key)|keyRep;reserved=false;
    }
    void Deactivate() {RestoreInputs();fire.Cancel();readyWindow.Cancel();pendingShot=false;pendingTarget=EntityHandle{};dumpTarget=EntityHandle{};EndPose();HideGun();displayReady=false;displayCharge=0;}
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        Deactivate();for(auto& s:shots) if(Entity* e=s.handle.getEntity()) e->release();
        shots.clear();impacts.clear();before.clear();owner=nullptr;fire={};
        animatedCombo=preparedDumpCombo=0;
    }
    // Only after native player teardown, as for Sam/knife archive references.
    void SceneReleased()
    {
        for(auto& r:resources)
        {
            if(r.bank) reinterpret_cast<void(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00D60)(r.id,&r.archive);
            if(r.pinned) g_ObjReadManager.removeReference(static_cast<eObjID>(r.id),0);
            if(r.requested) g_ObjReadManager.removeRequest(static_cast<eObjID>(r.id),0);
            r.bank=r.pinned=r.requested=false;
        }
    }
    void Tick(Pl0000* player,bool selected)
    {
        pendingShot=false;pendingTarget=EntityHandle{};
        if(owner!=player) {if(owner) Forget(owner);owner=player;}
        before.clear();
        if(!shots.empty() || selected)
        {
            auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
            for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
            {
                Entity* target=*it;if(!SamTargets::Enemy(target)) continue;
                HealthBefore entry;entry.target=target;entry.hp=static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp;
                before.push_back(entry);
            }
        }
        if(!selected) {Deactivate();return;}
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        const bool eligible=player && player->m_pEntity && SamNativeRuntime::Get().Owns(player) &&
            foreground==GetCurrentProcessId() && !gui::IsMenuVisible() && !g_StaFlags.STA_PAUSE &&
            EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,story,player->isBladeModeActive()!=FALSE);
        if(!eligible) {Deactivate();return;}
        Reserve();
        if(Stopped()) return;
        const bool held=(GetAsyncKeyState('C')&0x8000)!=0 || padOn!=0;
        auto move=player->isInAir()?SmgPolicy::Aerial:
            (player->m_CurrentInput.m_On&player->m_ButtonHeavyAttack)?SmgPolicy::Launcher:
            (GetAsyncKeyState('A')&0x8000 || GetAsyncKeyState('D')&0x8000)?SmgPolicy::Sweep:SmgPolicy::Burst;
        fire.Tick(held,true,move);
        fire.RejectUnfunded();
        displayCharge=fire.charge.frames;
        displayRounds=fire.rounds;displayReload=fire.reload;
        readyWindow.Tick(true,Neutral());
        if(gunPose && !Neutral()) EndPose();
        if(poseTicks && !OwnsPose())
        {aiming.Reset();poseNode=-1;poseTicks=0;gunPose=readyPose=poseFrozen=false;}
        else if(poseTicks)
        {
            // Hold the end of the native ready loop instead of leaking into
            // Raiden's sword idle between shots. Recoil itself runs at 1x.
            if(readyPose && readyWindow.remaining && poseTicks<=2)
            {if(!poseFrozen) player->setNodePlaybackSpeed(poseNode,0);poseFrozen=true;}
            else if(--poseTicks==0) {poseTicks=1;EndPose();}
        }
        displayReady=ResourcesReady() && Equip();
        if(!displayReady) return;
        if(!fire.Ready())
        {
            if(readyWindow.remaining && Neutral() && !poseTicks) Animate(false,true);
            AimAt(SmgPolicy::Magdump(fire.move)?dumpTarget.getEntity():Acquire(false));
            if(readyPose && !readyWindow.remaining && !fire.queued) EndPose();
            return;
        }
        if(fire.ordinal==0 && animatedCombo!=fire.combo)
        {
            animatedCombo=fire.combo;dumpTarget=EntityHandle{};
            if(SmgPolicy::Magdump(fire.move)) dumpTarget=Acquire(false);
            if(!fire.comboPose)
            {
                // Enemy shooting state: 2200 raise -> 2400 recoil -> 0400.
                // Starting directly with the nine-frame recoil leaves the
                // previous sword pose in the blend and points the gun upward.
                Animate(false,false,true);fire.delay=SmgPolicy::RaiseTicks;return;
            }
            Animate();if(fire.delay) return;
        }
        Entity* target=SmgPolicy::Magdump(fire.move)?dumpTarget.getEntity():Acquire(fire.move==SmgPolicy::Sweep);
        if(SmgPolicy::Magdump(fire.move) && (!SamTargets::Enemy(target) || static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp<=0))
        {target=Acquire(false);dumpTarget=target;}
        const auto round=SmgPolicy::RoundMove(fire.move,fire.ordinal);
        if(SmgPolicy::Magdump(fire.move))
        {
            // The raise clip can expire on the same tick as its delay. Track
            // preparation per dump, not by the current pose's lifetime, so a
            // completed raise cannot restart forever before bullet one.
            if(preparedDumpCombo!=fire.combo && Neutral())
            {preparedDumpCombo=fire.combo;Animate(false,false,true);fire.delay=SmgPolicy::RaiseTicks;return;}
            Animate(true);
        }
        else if(!fire.comboPose) Animate(true);
        AimAt(target);
        pendingTarget=target;pendingMove=round;pendingShot=true;
    }
    void PostTick(Pl0000* player)
    {
        if(owner!=player || !player || !player->m_pEntity || Stopped()) return;
        const bool react=player->isAlive() && !g_StaFlags.STA_PAUSE && !EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        // Native TickGame updates the hand constraint and weapon muzzle bones.
        // Emit afterward so the bullet/flash use the gun's rendered position,
        // including during a charge-to-shoot animation transition.
        if(pendingShot)
        {
            pendingShot=false;
            if(react && g_pPlayerManager && g_pPlayerManager->getSubWeaponEquipped()==NativeSmgMenuPolicy::Slot &&
                EncounterPolicy::CanThrow(player->m_Rno0,true,player->isInAir()!=FALSE,false,player->isBladeModeActive()!=FALSE))
            {
                Entity* target=pendingTarget.getEntity();
                if(SmgPolicy::Magdump(fire.move) && (!SamTargets::Enemy(target) || static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp<=0))
                {target=Acquire(false);dumpTarget=target;}
                if(Spawn(target,pendingMove)) {fire.Fired();readyWindow.Fired();displayRounds=fire.rounds;displayReload=fire.reload;}
            }
            else fire.Cancel();
        }
        for(auto& impact:impacts) if(impact.cooldown) --impact.cooldown;
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
        {
            Entity* target=*it;if(!SamTargets::Enemy(target) || !SamElectricPolicy::Ordinary(unsigned(target->m_ObjId))) continue;
            auto* actor=static_cast<BehaviorAppBase*>(target->m_pBehavior);
            const auto& hits=actor->m_AttackHits;if(!hits.m_vector || hits.m_size>hits.m_capacity || hits.m_size>128) continue;
            for(size_t h=0;h<hits.m_size;++h)
            {
                const auto& hit=hits.m_vector[h];const auto& data=hit.m_HitData;
                if(!SmgPolicy::OwnedHit(data.field_0,data.field_4,data.field_18.getEntity()==player->m_pEntity)) continue;
                bool identified=false;
                // Weak handle values still identify bullets released on collision.
                // Resolving vanished handles to nullptr would match unrelated hits.
                for(const auto& shot:shots)
                    if(!std::memcmp(&hit.field_120,&shot.handle,sizeof(EntityHandle)) ||
                        !std::memcmp(&hit.field_124,&shot.handle,sizeof(EntityHandle))) {identified=true;break;}
                bool lostHp=false;
                for(const auto& snapshot:before) if(snapshot.target.getEntity()==target) {lostHp=actor->m_Hp<snapshot.hp;break;}
                if(!react || !identified || !lostHp || actor->m_Hp<=0) continue;
                auto found=std::find_if(impacts.begin(),impacts.end(),[target](const auto& v){return v.target.getEntity()==target;});
                if(found==impacts.end()) {Impact v;v.target=target;impacts.push_back(v);found=impacts.end()-1;}
                if(found->cooldown) continue;
                if(data.field_0==SmgPolicy::LaunchAttack)
                {actor->m_TransSpeed.y=(std::max)(actor->m_TransSpeed.y,0.28f);found->cooldown=30;}
                else if(!actor->isOnGround())
                {actor->m_TransSpeed.y=(std::max)(actor->m_TransSpeed.y,0.06f);found->cooldown=4;}
            }
        }
        impacts.erase(std::remove_if(impacts.begin(),impacts.end(),[](const auto& v){return !v.target.getEntity() || !v.cooldown;}),impacts.end());
        shots.erase(std::remove_if(shots.begin(),shots.end(),[](auto& s){
            Entity* entity=s.handle.getEntity();if(!entity) return true;
            if(++s.age<SmgPolicy::Lifetime) return false;entity->release();return true;}),shots.end());
    }
};
