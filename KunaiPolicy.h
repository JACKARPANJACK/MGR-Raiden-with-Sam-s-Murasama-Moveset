#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace KunaiPolicy
{
    enum Variant { Native, Stun, Explosive, Heat, Count };
    constexpr unsigned Object = 0x30372;
    constexpr unsigned GrenadeObject = 0x31011;
    constexpr unsigned Attack = 0x151;
    constexpr int GrenadeSlot = 1;
    constexpr unsigned MaxShots = 32, Lifetime = 300;
    inline void Configure(unsigned char* data,int variant)
    {
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        auto scalar=[data](unsigned offset,float value){std::memcpy(data+offset,&value,4);};
        unsigned flags=0;std::memcpy(&flags,data,4);
        word(0,flags|4);word(4,Object);word(0x110,0x69);word(0x114,0x3C);
        word(0x10,Attack);word(0x14,variant==Stun?6:variant==Explosive?12:20);
        word(0x18,0);word(0x1C,20);scalar(0x160,0.38f);scalar(0x164,250.0f);
    }
    inline int Cycle(int value, int direction) { return (value + direction + Count) % Count; }
    inline int ImpactDamage(int variant) { return variant == Stun ? 6 : variant == Explosive ? 12 : 20; }
    inline int BurnDamage(int maxHp) { return (std::min)(3, (std::max)(1,maxHp / 100)); }
    inline bool CanSelect(unsigned state, bool alive, bool airborne, bool blocked, bool bladeMode)
    { return alive && !airborne && !blocked && !bladeMode && (state <= 4 || (state >= 0x100000 && state <= 0x100002)); }
    inline bool CanSuspend(unsigned state, bool alive, bool blocked, bool bladeMode)
    { return alive && !blocked && !bladeMode && state >= 0x100000 && state <= 0x100002; }
    inline bool CanResume(unsigned state, bool alive, bool aiming, unsigned elapsed)
    { return alive && !aiming && elapsed >= 3 && state <= 4; }
    inline bool Matches(unsigned attack, int damage, int variant, bool owned)
    { return owned && variant > Native && variant < Count && attack == Attack && damage == ImpactDamage(variant); }
    inline bool TimedDetonation(int variant, unsigned age, bool vanished)
    { return variant == Explosive && age >= 3 && (vanished || age >= 180); }
}
