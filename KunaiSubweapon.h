#pragma once
#include "KunaiPolicy.h"
#include "SamMovesetManager.h"
#include "SamElectricCombat.h"
#include <PlayerManagerImplement.h>
#include <atomic>
#include <array>
#include <vector>
#include <memory>

// Only the native Raiden grenade-release call is redirected. Native inventory,
// aiming, throw animation and ammunition checks/consumption remain in 7A4410.
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
    bool resourceRequested=false, suspended=false, wasPrevious=false, wasNext=false;
    unsigned suspendedTicks=0, waitTicks=0;
    bool installed=false;

    static bool Blocked()
    { return Trigger::StpFlags.STP_OBJ || g_StaFlags.STA_QTE || g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT; }
    static unsigned Handle(const EntityHandle& h)
    { unsigned raw=0; static_assert(sizeof(h)==sizeof(raw)); std::memcpy(&raw,&h,sizeof(raw)); return raw; }
    static float DistanceSquared(const Hw::cVec4& a,const Hw::cVec4& b)
    { const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z; }
    static Behavior* NativeCreate(Entity* actor,void* descriptor)
    { return reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(actor,descriptor); }
    void Detonate(Shot& shot)
    {
        if (shot.spent || !owner || !owner->m_pEntity || !owner->isAlive() || Blocked()) return;
        shot.spent=true;
        // A real native grenade payload supplies explosion collision, sound and
        // particles. Creating the payload does not consume a second inventory item.
        Descriptor payload=shot.grenade;
        *reinterpret_cast<unsigned*>(payload.data()+4)=KunaiPolicy::GrenadeObject;
        auto* hit=reinterpret_cast<CollisionAttackData::HitData*>(payload.data()+0x10);
        hit->field_4=0; hit->field_8=24; hit->field_C=20;
        Hw::cVec4 origin=shot.position, target=origin, rotation(0,0,0,0);
        target.y-=0.1f;
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float)>(shared::base+0x16E30)
            (payload.data(),&origin,&target,&rotation,0.01f,120.0f);
        *reinterpret_cast<unsigned*>(payload.data()+0x174)=0; // no stale target attachment
        NativeCreate(owner->m_pEntity,payload.data());
    }
    void Impact(Pl0000* player, Entity* target, Shot& shot)
    {
        if (shot.spent) return;
        shot.position=target->getTransPos();
        if (shot.variant==KunaiPolicy::Explosive) { Detonate(shot); return; }
        shot.spent=true;
        if (shot.variant==KunaiPolicy::Stun) electric.Stun(player,target,90);
        else if (shot.variant==KunaiPolicy::Heat)
        {
            bool present=false;
            for (auto& burn : burns) if (burn.target.getEntity()==target) {burn.remaining=90; present=true; break;}
            if (!present && burns.size()<KunaiPolicy::MaxShots) { Burn burn; burn.target=target; burns.push_back(burn); }
            // The native wp0372 bank remains responsible for the heat-blade
            // trail/impact; no Sam-only effect ID is applied to this projectile.
        }
    }
public:
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
    void Select(int variant) { if(installed && variant>=0 && variant<KunaiPolicy::Count) pending=variant; }
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        electric.Reset(); burns.clear(); beforeHits.clear();
        SamMovesetManager::Instance().SetSubweaponActive(false);
        for(auto& shot:shots) if(!shot->fade) {shot->visual.FadeUnits(3,0);shot->fade=30;shot->spent=true;}
        if(g_pPlayerManager && selected.load()!=0 && g_pPlayerManager->getSubWeaponEquipped()==KunaiPolicy::GrenadeSlot)
            g_pPlayerManager->setSubWeaponEquipped(previousSlot);
        if(resourceRequested) g_ObjReadManager.removeRequest(static_cast<eObjID>(KunaiPolicy::Object),0);
        resourceRequested=false; suspended=false; suspendedTicks=waitTicks=0;
        selected=0; pending=-1; owner=nullptr;
    }
    void Tick(Pl0000* player)
    {
        if(owner!=player) {if(owner) Forget(owner); owner=player;}
        if(!player || !g_pPlayerManager) return;
        beforeHits.clear();
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
        // Selecting a different native inventory item explicitly leaves kunai mode.
        if(Selected()!=0 && g_pPlayerManager->getSubWeaponEquipped()!=KunaiPolicy::GrenadeSlot)
        {selected=0;previousSlot=g_pPlayerManager->getSubWeaponEquipped();}
        int choice=Pending();
        if(choice>=0 && KunaiPolicy::CanSelect(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,Blocked(),player->isBladeModeActive()!=FALSE))
        {
            if(choice!=0)
            {
                if(!resourceRequested)
                    resourceRequested=g_ObjReadManager.requestObject(static_cast<eObjID>(KunaiPolicy::Object),0)!=FALSE;
                if(!resourceRequested || !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(KunaiPolicy::Object),0))
                {if(++waitTicks>600) {pending=-1;waitTicks=0;} return;}
                if(Selected()==0) previousSlot=g_pPlayerManager->getSubWeaponEquipped();
                g_pPlayerManager->setSubWeaponEquipped(KunaiPolicy::GrenadeSlot);
            }
            else if(Selected()!=0 && g_pPlayerManager->getSubWeaponEquipped()==KunaiPolicy::GrenadeSlot)
                g_pPlayerManager->setSubWeaponEquipped(previousSlot);
            selected=choice; pending=-1;waitTicks=0;
        }
        auto& sam=SamMovesetManager::Instance();
        SamMovesetManager::DebugSnapshot state{};sam.CopyDebugSnapshot(state);
        const bool aiming=(player->m_CurrentInput.m_On & player->m_ButtonUseSubweapon)!=0;
        // Sam's graph has no Raiden grenade states. Scope the original Raiden
        // graph to subweapon use, then restore Sam once native aim/throw ends.
        if(!suspended && Selected()!=0 && SamNativeRuntime::Get().Active(player) && aiming &&
            KunaiPolicy::CanSuspend(player->m_Rno0,player->isAlive()!=FALSE,Blocked()||state.roundTripActive,player->isBladeModeActive()!=FALSE))
        {
            const auto input=player->m_CurrentInput;
            SamNativeRuntime::Get().Deactivate(player);
            player->m_CurrentInput=input;
            suspended=true; suspendedTicks=0; sam.SetSubweaponActive(true);
        }
        if(suspended)
        {
            if(!sam.IsRequested() || !sam.IsEnabled() || !player->isAlive())
            {suspended=false;sam.SetSubweaponActive(false);}
            else if(!Blocked() && KunaiPolicy::CanResume(player->m_Rno0,true,aiming,++suspendedTicks) &&
                SamNativeRuntime::Get().Activate(player))
            {suspended=false;sam.SetSubweaponActive(false);}
        }
    }
    Behavior* Release(Entity* actor,void* original)
    {
        const int variant=Selected();
        auto* player=g_Scene.m_pPlayer;
        if(!player || player!=owner || actor!=player->m_pEntity || variant==0 || !original ||
            !g_pPlayerManager || g_pPlayerManager->getSubWeaponEquipped()!=KunaiPolicy::GrenadeSlot ||
            *reinterpret_cast<unsigned*>(static_cast<unsigned char*>(original)+4)!=KunaiPolicy::GrenadeObject ||
            !resourceRequested || !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(KunaiPolicy::Object),0) || shots.size()>=KunaiPolicy::MaxShots)
            return NativeCreate(actor,original);
        auto shot=std::make_unique<Shot>();
        std::memcpy(shot->grenade.data(),original,shot->grenade.size());
        Descriptor descriptor=shot->grenade;
        auto* data=descriptor.data();
        KunaiPolicy::Configure(data,variant);
        // Keep Raiden's friendly collision flags and owner/handle. Bladewolf's
        // hostile 0x10000180 flags and actor fields must never be copied here.
        Behavior* projectile=NativeCreate(actor,data);
        if(!projectile || !projectile->m_pEntity) return NativeCreate(actor,original);
        // Exact wp0372 post-create initialization from sub_16E2F0.
        auto* bytes=reinterpret_cast<unsigned char*>(projectile);
        *reinterpret_cast<int*>(bytes+0xBF4)=0;
        *reinterpret_cast<float*>(bytes+0xBF8)=0.5f;
        *reinterpret_cast<float*>(bytes+0xBFC)=0.3f;
        shot->projectile=projectile->m_pEntity;shot->position=projectile->m_pEntity->getTransPos();shot->variant=variant;
        if(variant==KunaiPolicy::Stun) SamElectricCombat::Lightning(player,projectile->m_pEntity,&shot->visual);
        shots.push_back(std::move(shot));
        return projectile;
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
            if(!shot->spent && KunaiPolicy::TimedDetonation(shot->variant,++shot->age,!projectile)) Detonate(*shot);
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
