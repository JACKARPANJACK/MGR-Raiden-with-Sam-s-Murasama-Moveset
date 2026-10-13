#pragma once
#include "GunChargePolicy.h"
#include "SamTargets.h"
#include "SamNativeRuntime.h"
#include "EncounterPolicy.h"
#include "gui.h"
#include "SamResourceManager.h"
#include "SamStagePolicy.h"
#include "GunRuntimeLog.h"
#include <cItemPossessionBase.h>
#include <cObjReadManager.h>
#include <array>

// Raiden RPG (slot 5) and Stinger (slot 6), using the retail 7A3EF0 descriptor
// and BulletBase factory independently of the ground-only aiming action graph.
class NativeLauncherSubweapon
{
    struct Resource {unsigned id;bool requested=false,pinned=false,bank=false;Hw::cFmerge archive;};
    std::array<Resource,8> resources{{{0x31002},{0x310E1},{0x31011},{0x31000},{0x310E0},{0x31031},{0x31041},{0x31051}}};
    EntityHandle fallbackWeapon;
    static constexpr unsigned Constraint=14;
    Pl0000* owner=nullptr;
    GunChargePolicy::Hold charge;
    unsigned pendingTier=0,cooldown=0,pendingAge=0;
    bool pending=false,reserved=false;
    bool nativeHeld=false;
    const char* lastStage="idle";
    void Trace(const char* stage)
    {
        if(!std::strcmp(lastStage,stage)) return;
        lastStage=stage;
        GunRuntimeLog::Write("AERIAL slot=%d stage=%s air=%d state=%08X node=%d ticks=%u nativeInput=%d",
            equipped,stage,owner?owner->isInAir()!=FALSE:0,owner?owner->m_Rno0:0,airNode,airTicks,nativeHeld);
    }
    int airNode=-1;
    unsigned airTicks=0,airRemaining=0;
    bool OwnsAirPose() const
    {
        return owner && airNode>=0 && owner->m_pAnimationSlot && owner->m_pAnimationSlot->m_pArray &&
            owner->m_pAnimationSlot->m_pArray[0].m_SlotId==airNode &&
            !std::strcmp(owner->m_pAnimationSlot->m_pArray[0].m_pAnimName,"Direct");
    }
    void EndAirPose()
    {
        if(OwnsAirPose() && owner->isAlive() && !owner->isBladeModeActive() &&
            !EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT) &&
            EncounterPolicy::CanThrow(owner->m_Rno0,true,owner->isInAir()!=FALSE,false,false))
        {
            if(owner->isInAir()) owner->requestAnimationByName("0232",0,0.05f,1,0,0,1);
            else if(owner->m_Rno0<=4) owner->requestAnimationByMap(4);
        }
        airNode=-1;airTicks=airRemaining=0;
    }
    bool BeginAirPose()
    {
        auto* inventory=Inventory();
        if(!inventory || inventory->noUse() || inventory->getEssentialPossession()<=0) return false;
        const auto clip=SamResourceManager::Instance().GetRaidenClip(GunChargePolicy::AirMotion(equipped));
        const unsigned length=SamStagePolicy::Length(clip.motion);
        if(!length || !owner->isInAir()) return false;
        // Motion only: native ground aim flags, root travel and ammo events must
        // never run here. The existing aerial graph retains gravity/landing.
        airNode=owner->setDirectAnimation(clip.motion,nullptr,0,0.05f,1,0x8000000,0,3);
        if(airNode<0) return false;
        airTicks=0;airRemaining=(length+2)/3;return true;
    }
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
    bool Load(Resource& r,bool effects)
    {
        const auto id=static_cast<eObjID>(r.id);
        if(!r.requested) r.requested=g_ObjReadManager.requestObject(id,0)!=FALSE;
        if(!r.requested || !g_ObjReadManager.isObjectLoaded(id,0)) return false;
        if(!r.pinned) {g_ObjReadManager.addUseRef(id,0);r.pinned=true;}
        if(!effects) return true; // Stinger projectile has no .eff member.
        if(!r.bank && g_ObjReadManager.getDataAtSet(r.archive,id,0))
            r.bank=reinterpret_cast<BOOL(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00C50)(r.id,&r.archive)!=FALSE;
        return r.bank;
    }
    bool Ready()
    {
        if(!Compatible()) return false;
        if(GunChargePolicy::GrenadeSlot(equipped)) return Load(resources[equipped==1?2:equipped+3],true);
        const bool projectile=Load(resources[equipped==6?1:0],equipped!=6);
        const bool model=Load(resources[equipped==6?4:3],true);
        return projectile && model;
    }
    void ReleaseWeapon()
    {
        Entity* weapon=fallbackWeapon.getEntity();
        if(owner && weapon && owner->getConstraintsEntity(Constraint)==weapon) owner->removeConstraint(Constraint);
        if(weapon) weapon->release();
        fallbackWeapon=EntityHandle{};
    }
    Entity* Weapon()
    {
        const unsigned model=equipped==6?0x310E0:0x31000;
        Entity* native=owner->m_SubWep2Handle.getEntity();
        if(native && native->m_pBehavior && unsigned(native->m_ObjId)==model)
        {ReleaseWeapon();return native;}
        if(Entity* weapon=fallbackWeapon.getEntity()) return weapon;
        if(owner->getConstraintsEntity(Constraint)) return nullptr;
        Entity* weapon=g_EntitySystem.createEntity("Pl0010_SUBWEP2",static_cast<eObjID>(model),nullptr);
        if(!weapon || !weapon->m_pBehavior) {if(weapon) weapon->release();return nullptr;}
        fallbackWeapon=weapon;weapon->m_pBehavior->m_pOwner=owner;
        owner->attachObject(Constraint,owner->m_pEntity,weapon,0x701,-1);
        weapon->m_pBehavior->onDisp();
        return weapon;
    }
    cItemPossessionBase* Inventory() const
    {
        return reinterpret_cast<cItemPossessionBase*(__thiscall*)(void*,unsigned)>(shared::base+0x54E5E0)
            (reinterpret_cast<void*>(shared::base+0x1486EA0),GunChargePolicy::GrenadeSlot(equipped)?GunChargePolicy::GrenadeAlias(equipped):GunChargePolicy::LauncherAlias(equipped));
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
        if(!inventory || inventory->noUse() || inventory->getEssentialPossession()<=0 || !params) return false;
        Entity* weapon=Weapon();
        if(!weapon || !weapon->m_pBehavior) return false;
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
    bool FireGrenade()
    {
        auto* inventory=Inventory();
        if(!inventory || inventory->noUse() || inventory->getEssentialPossession()<=0) return false;
        cParts* hand=owner->getPartsPtr(0x701);if(!hand) return false;
        auto origin=hand->getPos();origin.w=1;
        const float yaw=owner->m_Rot.y;
        Hw::cVec4 aim(origin.x+std::sin(yaw)*12,origin.y+1.5f,origin.z+std::cos(yaw)*12,1);
        Hw::cVec4 rotation(-0.125f,yaw,0,0);
        alignas(16) std::array<unsigned char,0x320> descriptor{};auto* data=descriptor.data();
        using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(data+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(data+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(data);
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        word(4,GunChargePolicy::GrenadeObject(equipped));
        word(0x110,equipped==2?0x52:equipped==3?0x53:equipped==4?0x51:0x26);
        word(0x114,equipped==2 || equipped==3?0x29:0x11);
        word(0x10,equipped==2?0x59:equipped==3?0x5A:equipped==4?0x58:0x57);word(0x14,0);word(0x18,150);word(0x1C,30);
        *reinterpret_cast<unsigned short*>(data+0x20)=0xA00;
        *reinterpret_cast<Entity**>(data+0x24)=owner->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=owner->m_pEntity;
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10000000 | (equipped==2?0x20:equipped==4?0x20000:0);
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x8800;
        word(0x170,reinterpret_cast<unsigned(__thiscall*)(Pl0000*)>(shared::base+0x5F8B40)(owner));
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float)>
            (shared::base+0x16E30)(data,&origin,&aim,&rotation,1.0f,120.0f);
        auto* bullet=reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(owner->m_pEntity,data);
        if(!bullet || !bullet->m_pEntity) return false;
        inventory->use();return true;
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
    void Cancel()
    {
        const bool mappedHeld=reserved?nativeHeld:
            owner && (owner->m_CurrentInput.m_On&owner->m_ButtonUseSubweapon)!=0;
        RestoreInputs();
        // Ground idle must not suppress the first new press after jumping.
        // A button held through cancellation still needs a fresh release.
        charge.Cancel(GunChargePolicy::AirInputHeld((GetAsyncKeyState('C')&0x8000)!=0,
            owner && (g_dbPad.m_On&(owner->m_ButtonUseSubweapon&~0xFu))!=0,mappedHeld));
        if(pending) Trace("cancelled");
        pending=false;pendingAge=0;EndAirPose();ReleaseWeapon();
    }
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
        if(!player || !player->m_pEntity || !GunChargePolicy::AerialSlot(slot,player->isInAir()!=FALSE) || !SamNativeRuntime::Get().Owns(player) ||
            foreground!=GetCurrentProcessId() || gui::IsMenuVisible() || g_StaFlags.STA_PAUSE || player->isBladeModeActive() ||
            !EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,story,false))
        {Cancel();return;}
        // Capture mapped mouse/keyboard input before Reserve removes it from
        // the native ground-only action graph. C remains an alternate binding.
        nativeHeld=(player->m_CurrentInput.m_On&player->m_ButtonUseSubweapon)!=0;
        Reserve();if(Trigger::StpFlags.STP_OBJ) return;
        if(airNode>=0)
        {
            if(!OwnsAirPose()) {Trace("pose-interrupted");Cancel();return;}
            ++airTicks;
            if(airRemaining && !--airRemaining)
            {
                // A delayed resource failure must not restart the throw pose.
                if(pending) {Cancel();return;}
                EndAirPose();
            }
        }
        if(cooldown) --cooldown;
        const bool held=GunChargePolicy::AirInputHeld((GetAsyncKeyState('C')&0x8000)!=0,padOn!=0,nativeHeld);
        if(held && !charge.down && !pending) Trace("press");
        if(charge.Tick(held,true,!cooldown && !pending && airNode<0))
        {pending=true;pendingTier=GunChargePolicy::Tier(charge.releasedFrames);pendingAge=0;Trace("queued");}
        const bool ready=Ready();
        if(!GunChargePolicy::GrenadeSlot(equipped) && ready) Weapon();
    }
    void PostTick(Pl0000* player)
    {
        if(player!=owner || !player || !pending || Trigger::StpFlags.STP_OBJ) return;
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        if(!g_pPlayerManager || g_pPlayerManager->getSubWeaponEquipped()!=equipped ||
            foreground!=GetCurrentProcessId() || gui::IsMenuVisible() || g_StaFlags.STA_PAUSE || player->isBladeModeActive() ||
            !EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,story,false) ||
            !player->isAlive() || !player->isInAir()) {Cancel();return;}
        if(!Ready()) {Trace("waiting-resource");if(++pendingAge>=60) Cancel();return;}
        if(airNode<0)
        {
            if(BeginAirPose()) Trace("pose-started");
            else {Trace("pose-unavailable-or-no-ammo");if(++pendingAge>=60) Cancel();}
            return;
        }
        if(!OwnsAirPose()) {Cancel();return;}
        if(airTicks<GunChargePolicy::AirReleaseTicks(equipped)) return;
        if(GunChargePolicy::GrenadeSlot(equipped)?FireGrenade():Fire())
        {Trace("spawned-ammo-spent");pending=false;cooldown=GunChargePolicy::GrenadeSlot(equipped)?30:GunChargePolicy::Cooldown(pendingTier);}
        // A failed release must not spawn late from a recovery/falling pose.
        else {Trace("spawn-failed-no-ammo-spent");pending=false;pendingAge=0;cooldown=30;}
    }
};
