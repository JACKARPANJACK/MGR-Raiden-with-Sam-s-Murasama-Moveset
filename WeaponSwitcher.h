#pragma once
#include "WeaponSwitchPolicy.h"
#include "SamMovesetManager.h"
#include "KunaiSubweapon.h"
#include <PlayerManagerImplement.h>
#include <EntitySystem.h>
#include <atomic>
#include <array>
#include "gui.h"

// Adapted native equip/attachment sequence from MGRWeaponSwitcher (Apache-2.0).
// All streaming and entity mutations run on TickGame, never a worker/render thread.
class WeaponSwitcher
{
    std::atomic<int> selected{0}, pending{-1};
    Pl0000* owner=nullptr;
    bool requested=false, previousDown=false, nextDown=false;
    unsigned requestId=0, waitTicks=0;
    unsigned previousPad=0;
    unsigned savedPadOn=0,savedPadTrig=0,savedPadRep=0;
    unsigned savedKeyOn=0,savedKeyTrig=0,savedKeyRep=0;
    bool inputsReserved=false;
    std::array<bool,WeaponSwitchPolicy::Count> warm{};
    unsigned padMask=0;
    int savedWheel=0,wheelRemainder=0;
    bool wheelReserved=false;
    static constexpr unsigned KeyMask=WeaponSwitchPolicy::ReservedKeys;
    void Cancel()
    {
        if (requested && requestId) g_ObjReadManager.removeRequest(static_cast<eObjID>(requestId),0);
        requested=false; requestId=waitTicks=0;
    }
    void ReleaseWarm()
    {
        for(int i=0;i<WeaponSwitchPolicy::Count;++i) if(warm[i])
            g_ObjReadManager.removeRequest(static_cast<eObjID>(WeaponSwitchPolicy::Weapons[i].object),0);
        warm.fill(false);wheelRemainder=0;
    }
    void Warm(int target)
    {
        for(int i=0;i<WeaponSwitchPolicy::Count;++i)
        {
            const unsigned object=WeaponSwitchPolicy::Weapons[i].object;
            if (warm[i] && i!=Selected() && i!=target)
            {g_ObjReadManager.removeRequest(static_cast<eObjID>(object),0);warm[i]=false;}
            if(i==target && object && !warm[i]) warm[i]=g_ObjReadManager.requestObject(static_cast<eObjID>(object),0)!=FALSE;
        }
        SamResourceManager::Instance().RequestAllSamResources();
    }
public:
    static WeaponSwitcher& Get() { static WeaponSwitcher s; return s; }
    int Selected() const { return selected.load(); }
    int Pending() const { return pending.load(); }
    void Select(int index) { if(index>=0 && index<WeaponSwitchPolicy::Count) pending=index; }
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        Cancel(); ReleaseWarm();pending=-1; selected=0; owner=nullptr;
    }
    void RestoreInputs()
    {
        if(!inputsReserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~padMask)|savedPadOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~padMask)|savedPadTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~padMask)|savedPadRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~KeyMask)|savedKeyOn;
        g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~KeyMask)|savedKeyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~KeyMask)|savedKeyRep;
        inputsReserved=false;
        padMask=0;
        if(wheelReserved) {g_Mouse.m_Wheel=savedWheel;wheelReserved=false;}
    }
    void ReserveUltimate(Pl0000* player)
    {
        if(!inputsReserved || !SamMovesetManager::Instance().IsEnabled() || !SamUltimatePolicy::ReserveButton(
            player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,
            g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT||Trigger::StpFlags.STP_OBJ,
            player->isBladeModeActive()!=FALSE,SamMovesetManager::Instance().IsSubweaponActive())) return;
        constexpr unsigned button=SamUltimatePolicy::ControllerButton;
        savedPadOn|=g_dbPad.m_On&button;savedPadTrig|=g_dbPad.m_Trig&button;savedPadRep|=g_dbPad.m_Rep&button;padMask|=button;
        g_dbPad.m_On&=~button;g_dbPad.m_Trig&=~button;g_dbPad.m_Rep&=~button;
        player->m_CurrentInput.m_On&=~button;player->m_CurrentInput.m_Trig&=~button;
    }
    void Tick(Pl0000* player)
    {
        if (owner!=player) { Cancel(); ReleaseWarm();pending=-1; selected=0; owner=player; }
        if(!player || !g_pPlayerManager) return;
        DWORD foreground=0;
        GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool prev=(GetAsyncKeyState('Q')&0x8000)!=0 || (GetAsyncKeyState(VK_OEM_4)&0x8000)!=0;
        const bool next=(GetAsyncKeyState('E')&0x8000)!=0 || (GetAsyncKeyState(VK_OEM_6)&0x8000)!=0;
        auto& sam=SamMovesetManager::Instance();
        // G/menu toggles and weapon selection share one source of truth.
        if(Pending()<0)
        {
            if((sam.IsRequested() || sam.IsEnabled()) && Selected()!=WeaponSwitchPolicy::Heatblades) selected=WeaponSwitchPolicy::Murasama;
            else if(Selected()==WeaponSwitchPolicy::Murasama) selected=WeaponSwitchPolicy::Sword;
        }
        const bool scripted=g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT;
        if(WeaponSwitchPolicy::AcceptInput(foreground==GetCurrentProcessId()&&!gui::IsMenuVisible(),player->isAlive()!=FALSE,scripted,Trigger::StpFlags.STP_OBJ))
        {
            const int current=Pending()<0?Selected():Pending();
            const unsigned padPress=0; // D-pad belongs to Raiden's native inventory.
            previousPad=(g_dbPad.m_On|g_dbPad.m_Trig)&0xF;
            const int input=WeaponSwitchPolicy::InputSelection(current,prev&&!previousDown,next&&!nextDown,padPress);
            if(input>=0) Select(input);
            const int wheel=WeaponSwitchPolicy::WheelSteps(g_Mouse.m_Wheel,wheelRemainder);
            if(wheel) Select(WeaponSwitchPolicy::Cycle(Pending()<0?Selected():Pending(),wheel));
            savedWheel=g_Mouse.m_Wheel;g_Mouse.m_Wheel=0;wheelReserved=true;
            // Q/E remain plugin weapon-cycle controls. Leave the D-pad
            // available to the native inventory, including the knife entry.
            {
                savedPadOn=savedPadTrig=savedPadRep=0;
                savedKeyOn=g_Keyboard.m_pOn[2]&KeyMask;
                savedKeyTrig=g_Keyboard.m_pTrig[2]&KeyMask;
                savedKeyRep=g_Keyboard.m_pRep[2]&KeyMask;
                g_Keyboard.m_pOn[2]&=~KeyMask; g_Keyboard.m_pTrig[2]&=~KeyMask; g_Keyboard.m_pRep[2]&=~KeyMask;
                if((prev&&!previousDown)||(next&&!nextDown))
                {
                    // Q/E take priority over native contextual actions on
                    // the selection press, including an input already mapped.
                    player->m_CurrentInput.m_Trig&=~(player->m_ButtonAction|player->m_ButtonUseItem|
                        player->m_ButtonUseSubweapon|player->m_ButtonBlademode|player->m_ButtonSwitchLockOn);
                }
                inputsReserved=true;
            }
        }
        else {previousPad=(g_dbPad.m_On|g_dbPad.m_Trig)&0xF;wheelRemainder=0;}
        previousDown=prev; nextDown=next;
        const int target=Pending();
        if(target<0) return;
        if(target==Selected()) {pending=-1;return;}
        SamMovesetManager::DebugSnapshot state{};
        SamMovesetManager::Instance().CopyDebugSnapshot(state);
        const bool recovery=SamUltimatePolicy::Attack(player->m_Rno0) && WeaponSwitchPolicy::Recovery(
            player->m_Rno0,player->getRemainingAnimationFrames(0),state.bossEnderActive);
        if(!WeaponSwitchPolicy::CanSwitch(player->m_Rno0,player->isAlive()!=FALSE,
            player->isInAir()!=FALSE,Trigger::StpFlags.STP_OBJ||g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT,
            player->isBladeModeActive()!=FALSE,state.roundTripActive||sam.IsSubweaponActive(),recovery)) return;
        const auto& weapon=WeaponSwitchPolicy::Weapons[target];
        Warm(target);
        // This menu entry is a shortcut to the native secondary inventory.
        // Equipping a knife must not release the currently equipped melee weapon.
        if (weapon.projectile)
        {
            if (!SamResourceManager::Instance().HeatbladesReady()) return;
            KunaiSubweapon::Get().Select(KunaiPolicy::Heatblades);
            pending=-1; Cancel(); Warm(Selected());
            return;
        }
        if(weapon.sam)
        {
            if(!SamNativeRuntime::Get().Owns(player)) {pending=-1;return;}
            SamResourceManager::Instance().RequestAllSamResources();
            SamResourceManager::Instance().Update();
            if(!SamResourceManager::Instance().IsRuntimeReady())
            { if(++waitTicks>600) {Cancel();pending=-1;} return; }
        }
        if(requested && requestId!=weapon.object) Cancel();
        if(weapon.object)
        {
            if(!warm[target]) return;
            if(!g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(weapon.object),0))
            { if(++waitTicks>600) { Cancel();pending=-1; } return; }
        }
        Entity* entity=nullptr;
        if(weapon.object && !weapon.projectile)
        {
            entity=g_EntitySystem.createEntity("Pl0010_SUBWEP1",static_cast<eObjID>(weapon.object),nullptr);
            if(!entity) { Cancel();pending=-1;return; }
        }
        // Leaving Murasama also cancels a pending Sam activation.
        if(!weapon.sam) sam.SetEnabled(player,false);
        g_pPlayerManager->setCustomWeaponEquipped(weapon.equipped);
        player->removeConstraint(5);
        reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base+0x77D0E0)(player);
        player->field_1420=player->m_CustomWeaponBone;
        // Preserve the actual sword/sheath entities. Native unarmed uses custom
        // weapon 5 + wp2040 and state 1, without the sword-lost hide/drop calls.
        player->m_SwordState=WeaponSwitchPolicy::SwordState(target);
        if(entity)
        {
            reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base+0x78AAE0)(player);
            player->removeConstraint(5);
            player->attachObject(5,player->m_pEntity,entity,player->m_CustomWeaponBone,-1);
            player->m_CustomWeaponHandle=entity;
            player->field_1420=player->m_CustomWeaponBone;
            if(entity->m_pBehavior) { entity->m_pBehavior->m_pOwner=player; entity->m_pBehavior->onDisp(); }
        }
        player->setRno(sam.IsEnabled()?0x100000:0,0,0,0);
        if(weapon.sam)
        {
            sam.SetEnabled(player,true);
            if(!sam.IsEnabled()) return;
        }
        selected=target; pending=-1; Cancel(); Warm(target);
    }
};
