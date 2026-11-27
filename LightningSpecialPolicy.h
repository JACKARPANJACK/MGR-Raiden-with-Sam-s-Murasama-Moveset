#pragma once
#include <cmath>
#include <algorithm>
namespace LightningSpecialPolicy
{
    constexpr unsigned First=20, Count=8;
    inline bool Is(unsigned move) { return move>=First && move<First+Count; }
    enum Shape { Thrust, TargetBurst, Radial, Fan, Spear, Wave };
    struct Spec { Shape shape; float range, width; unsigned pulses, stun, targets; };
    inline Spec Get(unsigned move)
    {
        switch(move) {
        case 20: return {Thrust,4,1.1f,1,60,1};
        case 21: return {TargetBurst,18,4,1,100,12};
        case 22: return {Radial,5,0,3,45,12};
        case 23: return {Fan,16,0.55f,1,25,6};
        case 24: return {Thrust,4,1.4f,2,40,1};
        case 25: return {Spear,14,0.65f,1,65,8};
        case 26: return {Wave,14,0.32f,1,35,8};
        case 27: return {Radial,8,0,4,50,12};
        default: return {Radial,0,0,0,0,0};
        }
    }
    // x/z are target offsets rotated into the player's local coordinates.
    inline bool Contains(Spec s,float x,float y,float z)
    {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||std::fabs(y)>3) return false;
        if(x*x+z*z>s.range*s.range) return false;
        if(s.shape==Radial) return true;
        if(z<0) return false;
        const float width=(s.shape==Fan||s.shape==Wave)?0.6f+z*s.width:s.width;
        return std::fabs(x)<=width;
    }
    // Long native clips contain recovery tails. Fire at their authored first
    // hit, not at a fraction of the full clip (which can be seconds too late).
    inline float PulseFrame(Spec s,unsigned pulse,unsigned length,float contact=8)
    {
        const float end=float((std::max)(2u,length)-1);
        const float first=(std::max)(1.0f,(std::min)(contact,end));
        const float spacing=(std::min)(12.0f,(end-first)/float((std::max)(1u,s.pulses-1)));
        return first+spacing*pulse;
    }
    inline int Damage(int native,int maxHp,unsigned pulses)
    {
        if(native<=0||maxHp<=0||!pulses) return 0;
        return (std::min)(native,(std::max)(1,(std::min)(60,maxHp/8)/int(pulses)));
    }
}
