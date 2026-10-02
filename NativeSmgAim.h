#pragma once
#include <array>
#include <cParts.h>

// The standalone bone-aim component embedded at Em0040+1350. It takes an
// EntityHandle and bone IDs, not an Em0040/AI pointer, so it can bind to Raiden.
class NativeSmgAim
{
    alignas(16) std::array<unsigned char,0xD8> work{};
    cParts* bone=nullptr;
    Hw::cMtx* previous=nullptr;
    unsigned short previousFlags=0;
    bool initialized=false;
public:
    void Reset()
    {
        if(bone && bone->m_pMulMtx==reinterpret_cast<Hw::cMtx*>(work.data()+0x40))
        {bone->m_pMulMtx=previous;bone->m_PartsFlag.m_Flag=(bone->m_PartsFlag.m_Flag&~4u)|(previousFlags&4u);}
        bone=nullptr;previous=nullptr;initialized=false;
    }
    bool Bind(Pl0000* player)
    {
        Reset();
        const unsigned char ctor[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0};
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x6831E0),ctor,sizeof(ctor))) return false;
        // Native gun aiming rotates spine marker 16 (71F878); keep the
        // character's own bone lookup, never copy enemy parts/IK pointers.
        bone=player->getPartsPtr(0x16);if(!bone) return false;
        previous=bone->m_pMulMtx;
        previousFlags=bone->m_PartsFlag.m_Flag;
        reinterpret_cast<void(__thiscall*)(void*)>(shared::base+0x6831E0)(work.data());
        reinterpret_cast<void(__thiscall*)(void*,Entity*,int,int)>(shared::base+0x682610)
            (work.data(),player->m_pEntity,0x16,-1);
        Hw::cVec4 offset(0,*reinterpret_cast<float*>(shared::base+0x123D0EC),0,0);
        const float limit=*reinterpret_cast<float*>(shared::base+0x123DB48);
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,float,float)>(shared::base+0x683270)
            (work.data(),&offset,limit,limit);
        *reinterpret_cast<float*>(work.data()+0xD4)=*reinterpret_cast<float*>(shared::base+0x123DA10);
        initialized=true;return true;
    }
    void Aim(const Hw::cVec4& target)
    {
        if(!initialized || !bone) return;
        Hw::cVec4 position=target;
        // Same component and update ABI used by the enemy's shooting controller
        // at 712A57/712A6E. Only its owned additive matrix touches Raiden.
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,int)>(shared::base+0x683330)
            (work.data(),&position,1);
    }
};
