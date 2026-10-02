#pragma once
#include "GunChargePolicy.h"
#include "SamTargets.h"
#include "SamNativeRuntime.h"
#include "EncounterPolicy.h"
#include "gui.h"
#include <cItemPossessionBase.h>
#include <cObjReadManager.h>
#include <array>

// Raiden RPG (slot 5) and Stinger (slot 6), using the retail 7A3EF0 descriptor
// and BulletBase factory independently of the ground-only aiming action graph.
class NativeLauncherSubweapon
{
    struct Resource {unsigned id;bool requested=false,pinned=false,bank=false;Hw::cFmerge archive;};
    std::array<Resource,2> resources{{{0x31002},{0x310E1}}};
    Pl0000* owner=nullptr;
    GunChargePolicy::Hold charge;
    unsigned pendingTier=0,cooldown=0,pendingAge=0;
    bool pending=false,reserved=false;
    int equipped=0;
    unsigned mask=0,padOn=0,padTrig=0,padRep=0,keyOn=0,keyTrig=0,keyRep=0;
    static constexpr unsigned Key=1u<<('C'%32);
    bool Compatible() const
    {
        const unsigned char rpg[]={0xC7,0x84,0x24,0xB4,0,0,0,2,0x10,3,0};
        const unsigned char stinger[]={0xC7,0x84,0x24,0xB4,0,0,0,0xE1,0x10,3,0};
        return !std::memcmp(reinterpret_cast<void*>(shared::base+0x7A40DC),rpg,sizeof(rpg)) &&
            !std::memcmp(reinterpret_cast<void*>(shared::base+0x7A4186),stinger,sizeof(stinger));
    }
    bool Ready()
    {
        if(!Compatible()) return false;
        auto& r=resources[equipped==6?1:0];const auto id=static_cast<eObjID>(r.id);
        if(!r.requested) r.requested=g_ObjReadManager.requestObject(id,0)!=FALSE;
        if(!r.requested || !g_ObjReadManager.isObjectLoaded(id,0)) return false;
        if(!r.pinned) {g_ObjReadManager.addUseRef(id,0);r.pinned=true;}
        if(!r.bank && g_ObjReadManager.getDataAtSet(r.archive,id,0))
            r.bank=reinterpret_cast<BOOL(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00C50)(r.id,&r.archive)!=FALSE;
        return r.bank;
    }
    cItemPossessionBase* Inventory() const
    {
        return reinterpret_cast<cItemPossessionBase*(__thiscall*)(void*,unsigned)>(shared::base+0x54E5E0)
            (reinterpret_cast<void*>(shared::base+0x1486EA0),GunChargePolicy::LauncherAlias(equipped));
    }
    Entity* Target() const
    {
        if(equipped==6)
        {
            Entity* lock=owner->field_26F0.m_TargetHandle.getEntity();
            if(SamTargets::Enemy(lock) && static_cast<BehaviorAppBase*>(lock->m_pBehavior)->m_Hp>0) return lock;
        }
        const auto pos=owner->m_pEntity->getTransPos();
        const float yaw=owner->m_Rot.y;
        Entity* best=nullptr;float nearest=100.0f*100.0f;
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
        {
            Entity* target=*it;if(!SamTargets::Enemy(target) || static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp<=0) continue;
            const auto p=target->getTransPos();const float dx=p.x-pos.x,dy=p.y-pos.y,dz=p.z-pos.z;
            if(dx*std::sin(yaw)+dz*std::cos(yaw)<0) continue;
            const float distance=dx*dx+dy*dy+dz*dz;
            if(distance<nearest) {nearest=distance;best=target;}
        }
        return best;
    }
    bool Fire()
    {
        auto* inventory=Inventory();auto* params=SamNativeRuntime::Get().NativeBattleParameters(owner);
        if(!inventory || inventory->getEssentialPossession()<=0 || !params) return false;
        // Keep the game's equipped RPG/Stinger model; never create a duplicate.
        Entity* weapon=owner->m_SubWep2Handle.getEntity();
        if(!weapon || !weapon->m_pBehavior) return false;
        const unsigned model=unsigned(weapon->m_ObjId);
        if(equipped==5 ? model!=0x31000 : (model!=0x310E0 && model!=0x31010)) return false;
        cParts* muzzle=weapon->m_pBehavior->getPartsPtr(0x800);
        if(!muzzle) return false;
        Hw::cVec4 origin=muzzle->getPos();
        Entity* target=Target();const float yaw=owner->m_Rot.y;
        Hw::cVec4 aim(origin.x+std::sin(yaw)*100,origin.y,origin.z+std::cos(yaw)*100,1);
        if(target) {aim=target->getTransPos();aim.y+=0.8f;}
        const float dx=aim.x-origin.x,dy=aim.y-origin.y,dz=aim.z-origin.z;
        Hw::cVec4 rot(-std::atan2(dy,std::sqrt(dx*dx+dz*dz)),std::atan2(dx,dz),0,0);
        alignas(16) std::array<unsigned char,0x320> descriptor{};auto* data=descriptor.data();
        using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(data+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(data+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(data);
        const int param=equipped==6?0xC9:0xC8;
        const int damage=GunChargePolicy::ChargedDamage(params->getAttackPower(param),pendingTier);
        if(damage<=0) return false;
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        word(4,GunChargePolicy::LauncherObject(equipped));word(0x110,equipped==6?0x48:0x27);word(0x114,0xE);
        word(0x10,0x55);word(0x14,damage);
        word(0x18,params->getAttackPowerHavokPow(param));word(0x1C,params->getAttackHavokMulScalar(param));
        data[0x20]=static_cast<unsigned char>(params->getHitStopTime(param));data[0x21]=0xA;
        *reinterpret_cast<Entity**>(data+0x24)=owner->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=owner->m_pEntity;
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10300000;
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x2008800;
        word(0x170,reinterpret_cast<unsigned(__thiscall*)(Pl0000*)>(shared::base+0x5F8B40)(owner));
        using Trajectory=void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float);
        reinterpret_cast<Trajectory>(shared::base+0x16E30)(data,&origin,&aim,&rot,equipped==6?0.6f:1.0f,200.0f);
        if(equipped==6 && target)
        {
            Hw::cVec4 offset(0,0.8f,0,0);
            reinterpret_cast<void(__thiscall*)(void*,Entity*,unsigned short,Hw::cVec4*)>(shared::base+0x3FED0)(data,target,0,&offset);
        }
        Behavior* bullet=reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(owner->m_pEntity,data);
        if(!bullet || !bullet->m_pEntity) return false;
        inventory->use(); // Native ammunition: exactly once, only on successful spawn.
        weapon->m_pBehavior->onDisp();
        return true;
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
    void RestoreInputs()
    {
        if(!reserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|padOn;g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|padTrig;g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|padRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~Key)|keyOn;g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~Key)|keyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~Key)|keyRep;reserved=false;
    }
    void Cancel() {RestoreInputs();charge.Cancel();pending=false;pendingAge=0;}
    void Forget(Pl0000* player)
    {if(player==owner) {Cancel();owner=nullptr;equipped=0;cooldown=0;}}
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
    void Tick(Pl0000* player,int slot)
    {
        if(owner!=player) {if(owner) Forget(owner);owner=player;}
        if(slot!=equipped) {Cancel();equipped=slot;cooldown=0;}
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        if(!GunChargePolicy::LauncherSlot(slot) || !player || !SamNativeRuntime::Get().Owns(player) ||
            foreground!=GetCurrentProcessId() || gui::IsMenuVisible() || g_StaFlags.STA_PAUSE || player->isBladeModeActive() ||
            !EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,story,false))
        {Cancel();return;}
        Reserve();if(Trigger::StpFlags.STP_OBJ) return;
        if(cooldown) --cooldown;
        const bool held=(GetAsyncKeyState('C')&0x8000)!=0 || padOn!=0;
        if(charge.Tick(held,true,!cooldown && !pending))
        {pending=true;pendingTier=GunChargePolicy::Tier(charge.releasedFrames);pendingAge=0;}
        if(!pending) {Ready();return;}
        if(Ready() && Fire()) {pending=false;cooldown=GunChargePolicy::Cooldown(pendingTier);}
        else if(++pendingAge>=60) {pending=false;pendingAge=0;}
    }
};
