#pragma once
#include <cstdint>
namespace WeaponSwitchPolicy
{
    struct Weapon { const char* name; unsigned object; int equipped; bool unarmed; bool sam; };
    constexpr int Sword=0, Murasama=1, Unarmed=5;
    inline constexpr Weapon Weapons[] = {
        {"Raiden sword",0,0,false,false}, {"Murasama / Sam moveset",0,0,false,true},
        {"Pole-arm",0x32000,2,false,false}, {"Sai",0x32030,3,false,false},
        {"Pincer blades",0x32020,4,false,false}, {"Unarmed",0,0,true,false}
    };
    constexpr int Count=sizeof(Weapons)/sizeof(Weapons[0]);
    constexpr int Cycle(int current,int delta) { return ((current+delta)%Count+Count)%Count; }
    constexpr unsigned ReservedKeys=(1u<<('Q'%32))|(1u<<('E'%32));
    constexpr bool AcceptInput(bool focused,bool alive,bool scripted,bool paused)
    { return focused && alive && !scripted && !paused; }
    constexpr unsigned PadPress(unsigned on,unsigned trig,unsigned previous)
    { return (on|trig)&~previous&0xFu; }
    constexpr int InputSelection(int current,bool previous,bool next,unsigned padTrig)
    {
        // Up/Down are direct sword presets; Left/Right cycle every weapon.
        if(padTrig&0x8) return Murasama;
        if(padTrig&0x4) return Sword;
        const bool back=previous || (padTrig&0x1)!=0;
        const bool forward=next || (padTrig&0x2)!=0;
        return back==forward ? -1 : Cycle(current,forward?1:-1);
    }
    constexpr bool CanSwitch(uint32_t state,bool alive,bool airborne,bool scripted,bool blade,bool flight)
    {
        if (!alive || airborne || scripted || blade || flight) return false;
        return state<=4 || state==0x100000 || state==0x100001 || state==0x100002;
    }
}
