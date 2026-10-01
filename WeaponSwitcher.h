#pragma once
#include "WeaponSwitchPolicy.h"
#include "SamMovesetManager.h"
#include <PlayerManagerImplement.h>
#include <EntitySystem.h>
#include <atomic>

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
    static constexpr unsigned KeyMask=WeaponSwitchPolicy::ReservedKeys;
    void Cancel()
    {
        if (requested && requestId) g_ObjReadManager.removeRequest(static_cast<eObjID>(requestId),0);
        requested=false; requestId=waitTicks=0;
    }
public:
    static WeaponSwitcher& Get() { static WeaponSwitcher s; return s; }
    int Selected() const { return selected.load(); }
    int Pending() const { return pending.load(); }
    void Select(int index) { if(index>=0 && index<WeaponSwitchPolicy::Count) pending=index; }
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        Cancel(); pending=-1; selected=0; owner=nullptr;
    }
    void RestoreInputs()
    {
        if(!inputsReserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~0xFu)|savedPadOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~0xFu)|savedPadTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~0xFu)|savedPadRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~KeyMask)|savedKeyOn;
        g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~KeyMask)|savedKeyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~KeyMask)|savedKeyRep;
        inputsReserved=false;
    }
    void Tick(Pl0000* player)
    {
        if (owner!=player) { Cancel(); pending=-1; selected=0; owner=player; }
        if(!player || !g_pPlayerManager) return;
        DWORD foreground=0;
        GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool prev=(GetAsyncKeyState('Q')&0x8000)!=0 || (GetAsyncKeyState(VK_OEM_4)&0x8000)!=0;
        const bool next=(GetAsyncKeyState('E')&0x8000)!=0 || (GetAsyncKeyState(VK_OEM_6)&0x8000)!=0;
        auto& sam=SamMovesetManager::Instance();
        // G/menu toggles and weapon selection share one source of truth.
        if(Pending()<0)
        {
            if(sam.IsRequested() || sam.IsEnabled()) selected=WeaponSwitchPolicy::Murasama;
            else if(Selected()==WeaponSwitchPolicy::Murasama) selected=WeaponSwitchPolicy::Sword;
        }
        const bool scripted=g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT;
        if(WeaponSwitchPolicy::AcceptInput(foreground==GetCurrentProcessId(),player->isAlive()!=FALSE,scripted,Trigger::StpFlags.STP_OBJ))
        {
            const int current=Pending()<0?Selected():Pending();
            const unsigned padPress=WeaponSwitchPolicy::PadPress(g_dbPad.m_On,g_dbPad.m_Trig,previousPad);
            previousPad=(g_dbPad.m_On|g_dbPad.m_Trig)&0xF;
            const int input=WeaponSwitchPolicy::InputSelection(current,prev&&!previousDown,next&&!nextDown,padPress);
            if(input>=0) Select(input);
            // Reserve these controls during ordinary gameplay so the native
            // D-pad inventory menu / Q,E bindings do not also fire. Keep QTE
            // and story inputs native. Restore the shared state after TickGame.
            {
                savedPadOn=g_dbPad.m_On&0xF; savedPadTrig=g_dbPad.m_Trig&0xF; savedPadRep=g_dbPad.m_Rep&0xF;
                savedKeyOn=g_Keyboard.m_pOn[2]&KeyMask;
                savedKeyTrig=g_Keyboard.m_pTrig[2]&KeyMask;
                savedKeyRep=g_Keyboard.m_pRep[2]&KeyMask;
                g_dbPad.m_On&=~0xFu; g_dbPad.m_Trig&=~0xFu; g_dbPad.m_Rep&=~0xFu;
                g_Keyboard.m_pOn[2]&=~KeyMask; g_Keyboard.m_pTrig[2]&=~KeyMask; g_Keyboard.m_pRep[2]&=~KeyMask;
                player->m_CurrentInput.m_On&=~0xFu;
                player->m_CurrentInput.m_Trig&=~0xFu;
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
        else previousPad=(g_dbPad.m_On|g_dbPad.m_Trig)&0xF;
        previousDown=prev; nextDown=next;
        const int target=Pending();
        if(target<0) return;
        SamMovesetManager::DebugSnapshot state{};
        SamMovesetManager::Instance().CopyDebugSnapshot(state);
        if(!WeaponSwitchPolicy::CanSwitch(player->m_Rno0,player->isAlive()!=FALSE,
            player->isInAir()!=FALSE,Trigger::StpFlags.STP_OBJ||g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT,
            player->isBladeModeActive()!=FALSE,state.roundTripActive)) return;
        const auto& weapon=WeaponSwitchPolicy::Weapons[target];
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
            if(!requested)
            {
                if(!g_ObjReadManager.requestObject(static_cast<eObjID>(weapon.object),0)) { pending=-1; return; }
                requestId=weapon.object; requested=true;
            }
            if(!g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(weapon.object),0))
            { if(++waitTicks>600) { Cancel();pending=-1; } return; }
        }
        Entity* entity=nullptr;
        if(weapon.object)
        {
            entity=g_EntitySystem.createEntity("Pl0010_SUBWEP1",static_cast<eObjID>(weapon.object),nullptr);
            if(!entity) { Cancel();pending=-1;return; }
        }
        // Leaving Murasama also cancels a pending Sam activation.
        if(!weapon.sam) sam.SetEnabled(player,false);
        g_pPlayerManager->setCustomWeaponEquipped(weapon.equipped);
        reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base+0x77D0E0)(player);
        player->setSwordLost(weapon.unarmed?TRUE:FALSE);
        if(entity)
        {
            reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base+0x78AAE0)(player);
            player->removeConstraint(5);
            player->attachObject(5,player->m_pEntity,entity,player->m_CustomWeaponBone,-1);
            player->m_CustomWeaponHandle=entity;
            if(entity->m_pBehavior) { entity->m_pBehavior->m_pOwner=player; entity->m_pBehavior->onDisp(); }
        }
        player->setRno(sam.IsEnabled()?0x100000:0,0,0,0);
        if(weapon.sam)
        {
            sam.SetEnabled(player,true);
            if(!sam.IsEnabled()) return;
        }
        selected=target; pending=-1; Cancel();
    }
};
