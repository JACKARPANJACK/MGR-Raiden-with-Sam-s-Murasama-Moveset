#pragma once
#include "LightningSpecialPolicy.h"
#include "SamVisualEffects.h"
#include "NativeLightningBank.h"
// Matches the dedicated pl1c30.eff/pl1c30.eft bank.
namespace CustomLightningEffects
{
    constexpr int Bank=NativeLightningBank::Id;
    constexpr int FirstCharge=950, FirstRelease=958, Impact=966;
    inline int Charge(unsigned move) {return LightningSpecialPolicy::Is(move)?FirstCharge+int(move-20):-1;}
    inline int Release(unsigned move) {return LightningSpecialPolicy::Is(move)?FirstRelease+int(move-20):-1;}
    constexpr int Miss=967;
    inline bool Spawn(Entity* anchor,int effect,cEspControler* controller,int bone=-1)
    {
        if(!anchor || !anchor->m_pBehavior || effect<0 || !NativeLightningBank::Get().Ready()) return false;
        return SamVisualEffects::Spawn(anchor->m_pBehavior,anchor,Bank,effect,controller,nullptr,bone);
    }
}
