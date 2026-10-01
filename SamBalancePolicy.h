#pragma once
#include <algorithm>
#include <cstdint>
namespace SamBalancePolicy
{
    constexpr float AttackSpeed = 1.20f;
    constexpr float RunSpeed = 1.20f;
    inline int Damage(int nativePower, unsigned number, bool addon, unsigned boxes, int targetMaxHp)
    {
        if (nativePower <= 0) return 0;
        const bool charged = number == 26 || number == 27;
        const bool heavy = number >= 12;
        int cap = charged ? 60 : heavy ? 35 : 20;
        if (targetMaxHp > 0)
        {
            int fraction = addon ? (targetMaxHp / 5) / int((std::max)(1u,boxes)) :
                targetMaxHp / (charged ? 12 : heavy ? 20 : 30);
            cap = (std::min)(cap,(std::max)(1,fraction));
        }
        return (std::min)(cap,(std::max)(1,int(nativePower * 0.85f)));
    }
    inline int ProjectileDamage(int nativePower, int targetMaxHp)
    { return (std::min)(Damage(nativePower,4,false,1,targetMaxHp), (std::max)(1,targetMaxHp / 50)); }
}
