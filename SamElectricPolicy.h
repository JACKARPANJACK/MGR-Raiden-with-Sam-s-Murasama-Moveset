#pragma once
#include <cstdint>
namespace SamElectricPolicy
{
    inline bool Ordinary(uint32_t model)
    {
        switch (model)
        {
        case 0x20010: case 0x20140: case 0x20142: case 0x20144:
        case 0x20150: case 0x20152: case 0x20160: case 0x20170: return true;
        default: return false;
        }
    }
    inline bool ConfirmedHit(int previousHp, int hp, bool attacking, bool owned, float distance)
    { return previousHp > hp && hp > 0 && attacking && owned && distance <= 5.0f; }
    inline bool Proc(unsigned roll, bool heavy, unsigned cooldown)
    { return cooldown == 0 && roll < (heavy ? 25u : 12u); }
    inline bool Finisher(int hp, int maxHp, float distance)
    { return hp > 0 && maxHp > 0 && hp <= maxHp / 4 && distance <= 3.0f; }
}
