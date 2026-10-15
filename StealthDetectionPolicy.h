#pragma once
#include "StealthStancePolicy.h"
namespace StealthDetectionPolicy {
using Stance=StealthStancePolicy::Stance;
constexpr unsigned RevealTicks=120;
inline unsigned UpdateReveal(unsigned previous,bool noisy,bool paused) {
    if(paused)return previous;
    return noisy?RevealTicks:previous?previous-1:0;
}
inline Stance Profile(Stance stance,bool groundOrEvading,bool alive,bool paused,bool scripted,unsigned reveal) {
    return groundOrEvading&&alive&&!paused&&!scripted&&!reveal?stance:Stance::Standing;
}
inline float RestoreRange(float current,float original,float applied,int beforeState,int afterState) {
    return beforeState==afterState&&current==applied?original:current;
}
inline bool Ordinary(unsigned id) {
    switch(id) {
    case 0x20010:case 0x20140:case 0x20142:case 0x20144:
    case 0x20150:case 0x20152:case 0x20160:case 0x20170:return true;
    default:return false;
    }
}
inline bool Reduce(unsigned id,int cautionState,Stance stance) {
    return Ordinary(id) && (cautionState==1||cautionState==2) && stance!=Stance::Standing;
}
inline float RangeSquared(float original,Stance stance) {
    if(!std::isfinite(original)||original<=0 || stance==Stance::Standing)return original;
    const float factor=stance==Stance::Prone?.25f:.45f;
    const float minimum=stance==Stance::Prone?1.5625f:4.0f;
    return (std::min)(original,(std::max)(minimum,original*factor*factor));
}
}
