#pragma once
#include "SamElectricPolicy.h"
namespace SamExecutionPolicy
{
    constexpr unsigned Human = 0x100066, Mech = 0x100067, ParryCounter = 0x10001F;
    inline bool Native(unsigned state) { return state == Human || state == Mech; }
    inline bool Finisher(unsigned move) { return move == 1 || move == 3 || move == 10 || move == 11; }
    inline bool Eligible(unsigned model, int hp, int maxHp, float distance, bool ready, bool occupied)
    { return SamElectricPolicy::Ordinary(model) && SamElectricPolicy::Finisher(hp,maxHp,distance) && ready && !occupied; }
    struct Counter
    {
        bool attempted = false;
        bool Poll(unsigned state, bool confirmedHit)
        {
            if (state != ParryCounter) { attempted=false; return false; }
            if (!confirmedHit || attempted) return false;
            attempted=true; return true;
        }
        void Reset() { attempted=false; }
    };
}
