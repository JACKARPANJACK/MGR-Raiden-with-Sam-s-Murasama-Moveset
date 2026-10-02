#pragma once
#include <cstdint>
#include "SamConfig.h"

namespace WeaponSwitchPolicy
{
    struct Weapon { const char* name; unsigned object; int equipped; bool unarmed; bool sam; bool projectile=false; };
    constexpr int Sword=0, Murasama=1, Unarmed=5, Heatblades=6;
    inline constexpr Weapon Weapons[] = {
        {"Raiden sword",0,0,false,false}, {"Murasama / Sam moveset",0,0,false,true},
        {"Pole-arm",0x32000,2,false,false}, {"Sai",0x32030,3,false,false},
        {"Pincer blades",0x32020,4,false,false}, {"Unarmed",0x32040,5,true,false},
        {"Bladewolf heatblades",0x30372,0,false,false,true}
    };
    constexpr int Count=sizeof(Weapons)/sizeof(Weapons[0]);
    // Native prologue/unarmed state 1; state 2 is the sword-lost path.
    constexpr int SwordState(int selected) { return selected==Unarmed?1:0; }
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

    inline unsigned PadConsumeMask()
    {
        unsigned mask = 0;
        if (SamConfig::GetInt("WeaponSwitch", "CycleNextDPadDown", 0)) mask |= 0x4; // Down
        if (SamConfig::GetInt("WeaponSwitch", "CycleNextDPadRight", 0)) mask |= 0x2; // Right
        if (SamConfig::GetInt("WeaponSwitch", "CyclePrevDPadLeft", 0)) mask |= 0x8; // Left
        if (SamConfig::GetInt("WeaponSwitch", "CyclePrevDPadUp", 0)) mask |= 0x1; // Up
        return mask;
    }

    inline int InputSelection(int current,bool previous,bool next,unsigned padTrig)
    {
        static int downNext = -1, rightNext = -1, leftPrev = -1, upPrev = -1;
        if (downNext == -1)
        {
            downNext = SamConfig::GetInt("WeaponSwitch", "CycleNextDPadDown", 1);
            rightNext = SamConfig::GetInt("WeaponSwitch", "CycleNextDPadRight", 0);
            leftPrev = SamConfig::GetInt("WeaponSwitch", "CyclePrevDPadLeft", 0);
            upPrev = SamConfig::GetInt("WeaponSwitch", "CyclePrevDPadUp", 0);
        }
        
        bool back = previous || (upPrev && (padTrig & 0x1)) || (leftPrev && (padTrig & 0x8));
        bool forward = next || (downNext && (padTrig & 0x4)) || (rightNext && (padTrig & 0x2));
        return back==forward ? -1 : Cycle(current,forward?1:-1);
    }
    constexpr bool CanSwitch(uint32_t state,bool alive,bool airborne,bool scripted,bool blade,bool flight,bool recovery=false)
    {
        if (!alive || airborne || scripted || blade || flight) return false;
        return state<=4 || state==0x100000 || state==0x100001 || state==0x100002 || recovery;
    }
}
