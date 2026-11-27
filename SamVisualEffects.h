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
    inline bool Spawn(Behavior* owner, Entity* anchor, int bank, int number, cEspControler* controller, Entity* blade = nullptr,int bone=-1)
    {
        if (!owner || !anchor) return false;
        alignas(16) unsigned char info[0x150]{};
        // Native createEffect passes (effect, owner, container index=0).
        // The final argument is NOT a parent pointer: entity parents are bound
        // below. A pointer here corrupts the descriptor's container selector.
        reinterpret_cast<void*(__thiscall*)(void*,int,Behavior*,int)>(shared::base + 0x39A0)(info,number,owner,0);
        reinterpret_cast<void(__thiscall*)(void*,cEspControler*)>(shared::base + 0x9FFB20)(info,controller);
        // A002F0 explicitly rejects parent entries whose bone is not -1.
        // Bind a hand through the supported matrix override, not that field.
        reinterpret_cast<void(__thiscall*)(void*,Entity*,int,int)>(shared::base + 0xA02140)(info,anchor,-1,0);
        if(bone>=0) {
            auto* part=owner->getPartsPtr(unsigned(bone));
            if(!part)return false;
            reinterpret_cast<void(__thiscall*)(void*,const Hw::cMtx*)>(shared::base+0x9FFAC0)(info,&part->m_LocalMatrix);
        }
        if (blade) reinterpret_cast<void(__thiscall*)(void*,Entity*,int)>(shared::base + 0xA03080)(info,blade,1);
        return reinterpret_cast<BOOL(__thiscall*)(Behavior*,int,void*)>(shared::base + 0x68C8B0)(owner,bank,info)!=FALSE;
    }
}
