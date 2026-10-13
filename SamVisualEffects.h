#pragma once
#include <Pl0000.h>
#include "SamEffectBanks.h"
namespace SamVisualEffects
{
    inline int ChargeNumber(int number)
    {
        switch (number)
        {
        case 539: return 117;
        case 540: return 118;
        case 541: return 119;
        case 550: return 121;
        default: return number;
        }
    }
    inline int Bank(int number)
    {
        return SamEffectBanks::Bank(number);
    }
    inline void Spawn(Behavior* owner, Entity* anchor, int bank, int number, cEspControler* controller, Entity* blade = nullptr)
    {
        if (!owner || !anchor) return;
        alignas(16) unsigned char info[0x150]{};
        const int parent = reinterpret_cast<int(__thiscall*)(Entity*)>(shared::base + 0x67C8A0)(anchor);
        reinterpret_cast<void*(__thiscall*)(void*,int,Behavior*,int)>(shared::base + 0x39A0)(info,number,owner,parent);
        reinterpret_cast<void(__thiscall*)(void*,cEspControler*)>(shared::base + 0x9FFB20)(info,controller);
        reinterpret_cast<void(__thiscall*)(void*,Entity*,int)>(shared::base + 0xA03080)(info,anchor,0);
        if (blade) reinterpret_cast<void(__thiscall*)(void*,Entity*,int)>(shared::base + 0xA03080)(info,blade,1);
        reinterpret_cast<void(__thiscall*)(Behavior*,int,void*)>(shared::base + 0x68C8B0)(owner,bank,info);
    }
}
