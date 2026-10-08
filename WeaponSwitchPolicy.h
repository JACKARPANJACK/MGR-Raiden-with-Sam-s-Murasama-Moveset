#pragma once
#include <cstdint>
#include "SamConfig.h"

namespace WeaponSwitchPolicy
{
    struct Weapon { const char* name; unsigned object; int equipped; bool unarmed; bool sam; };
    constexpr int Sword=0, Polearm=1, Sai=2, Pincers=3, DualBlades=4, Murasama=5, Unarmed=6;
    inline constexpr Weapon Weapons[] = {
        {"Raiden sword",0,0,false,false},
        {"L'Etranger (Pole-arm)",0x32000,2,false,false},
        {"Sai",0x32030,3,false,false},
        {"Pincer blades",0x32020,4,false,false},
        // Special Weapon 6 is plugin-owned; use native sword combat rather
        // than indexing the game's six-entry custom weapon tables with 6.
        {"Dual swords (wp2050) / Special Weapon 6",0x30050,6,false,false},
        {"Murasama / Sam moveset",0,0,false,true},
        {"Unarmed",0x32040,5,true,false}
    };
    constexpr int Count=sizeof(Weapons)/sizeof(Weapons[0]);
    constexpr int NativeEquipped(int type) { return type==6?0:type; }
    // Native prologue/unarmed state 1; state 2 is the sword-lost path.
    constexpr int SwordState(int selected) { return selected==Unarmed?1:0; }
    constexpr bool IsCustomWeaponIndex(int index) {
        return index==Polearm || index==Sai || index==Pincers || index==Unarmed;
    }
    constexpr int IndexFromCustomEquipped(int customEquipped)
    {
        switch(customEquipped) {
            case 2: return Polearm;
            case 3: return Sai;
            case 4: return Pincers;
            case 5: return Unarmed;
            default: return Sword;
        }
    }
    inline int WheelSteps(int delta,int& remainder)
    {
        delta=delta>1200?1200:delta< -1200?-1200:delta;
        remainder+=delta;
        const int steps=remainder/120;remainder%=120;return -steps; // up previous, down next
    }
    inline bool Recovery(unsigned state,float remaining,bool activeUltimate)
    {
        return !activeUltimate && state>=0x10000F && state<=0x10001A &&
            state!=0x100015 && state!=0x100016 && remaining>=0 && remaining<=6.0f;
    }
    constexpr int Cycle(int current,int delta) { return ((current+delta)%Count+Count)%Count; }
    constexpr unsigned ReservedKeys=(1u<<('Q'%32))|(1u<<('E'%32));
    constexpr bool AcceptInput(bool focused,bool alive,bool scripted,bool paused)
    { return focused && alive && !scripted && !paused; }
    constexpr unsigned PadPress(unsigned on,unsigned trig,unsigned previous)
    { return (on|trig)&~previous&0xFu; }
    constexpr unsigned DPadDown=0x4;
    struct PadBindings { unsigned forward; unsigned back; };
    inline const PadBindings& Bindings()
    {
        static const PadBindings bindings{
            DPadDown | (SamConfig::GetInt("WeaponSwitch","CycleNextDPadRight",0)?0x2u:0u),
            (SamConfig::GetInt("WeaponSwitch","CyclePrevDPadLeft",0)?0x8u:0u) |
            (SamConfig::GetInt("WeaponSwitch","CyclePrevDPadUp",0)?0x1u:0u)
        };
        return bindings;
    }

    inline unsigned PadConsumeMask()
    {
        return Bindings().forward | Bindings().back;
    }

    constexpr int InputSelectionWithBindings(int current,bool previous,bool next,unsigned padTrig,PadBindings bindings)
    {
        bool back = previous || (padTrig & bindings.back);
        bool forward = next || (padTrig & bindings.forward);
        return back==forward ? -1 : Cycle(current,forward?1:-1);
    }
    inline int InputSelection(int current,bool previous,bool next,unsigned padTrig)
    { return InputSelectionWithBindings(current,previous,next,padTrig,Bindings()); }
    constexpr bool CanSwitch(uint32_t state,bool alive,bool airborne,bool scripted,bool blade,bool flight,bool recovery=false)
    {
        if (!alive || airborne || scripted || blade || flight) return false;
        return state<=4 || state==0x100000 || state==0x100001 || state==0x100002 || recovery;
    }
}
