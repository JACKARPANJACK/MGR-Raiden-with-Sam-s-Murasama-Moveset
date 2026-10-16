#pragma once
#include <BehaviorEmBase.h>
#include <cstring>
namespace PistolNative {
constexpr unsigned DownState=0xA0004,GroundLoop=3,GetUp=4;
inline bool Compatible() {
    const unsigned char setter[]={0x53,0x55,0x56,0x8B,0xF1};
    const unsigned char down[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x83,0xEC,0x14};
    return !std::memcmp(reinterpret_cast<void*>(shared::base+0x739F00),setter,sizeof(setter)) &&
        !std::memcmp(reinterpret_cast<void*>(shared::base+0x73D300),down,sizeof(down)) &&
        *reinterpret_cast<unsigned*>(shared::base+0x753024+4*4)==shared::base+0x752EB2;
}
inline bool KnockDown(BehaviorEmBase* actor) {
    if(!actor || !Compatible())return false;
    // Retail Em0010 state dispatcher: A0004 -> 73D300. Native state entry
    // owns attack cancellation, fall physics, floor loop and recovery.
    reinterpret_cast<void(__thiscall*)(BehaviorEmBase*,unsigned,int,int,int)>
        (shared::base+0x739F00)(actor,DownState,0,0,0);
    return actor->m_Rno0==DownState;
}
}
