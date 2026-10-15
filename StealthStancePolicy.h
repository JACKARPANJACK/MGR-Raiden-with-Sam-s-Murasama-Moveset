#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace StealthStancePolicy {
enum class Stance : unsigned { Standing, Crouch, Prone };
constexpr unsigned R3=0x8000, L3=0x1000;
inline const char* Name(Stance stance) {
    return stance==Stance::Crouch?"Crouch":stance==Stance::Prone?"Prone":"Standing";
}
inline bool Locomotion(unsigned action) {return action<=3 || (action>=0x100000&&action<=0x100002);}
inline bool NativeWeaponSlot(int slot){return slot>=1 && slot<=6;}
inline bool GroundWeaponAction(unsigned action){return action==0x38 || action==0x39 || action==0x3A || action==0x3B || action==0x3D;}
inline int BaseClip(Stance stance,bool moving){return stance==Stance::Standing?-1:(stance==Stance::Crouch?0:2)+(moving?1:0);}
// Native bindings may include low direction bits as modifiers. Direction
// changes are neither action-button presses nor loud combat input.
inline unsigned ButtonBits(unsigned binding) {return binding&~0xFu;}
inline float StepLimit(Stance stance) {
    return stance==Stance::Crouch?.035f:stance==Stance::Prone?.013333333f:0.0f;
}
inline float MovementScale(Stance stance,float distance) {
    const float limit=StepLimit(stance);
    return limit>0 && distance>limit ? limit/distance:1.0f;
}
class Cycle {
    bool previous=false;
    Stance selected=Stance::Standing;
public:
    Stance Selected() const {return selected;}
    void Reset() {selected=Stance::Standing;previous=false;}
    void Set(Stance stance) {selected=stance;}
    bool Update(unsigned on,unsigned trig,bool allowed) {
        const bool down=((on|trig)&R3)!=0;
        const bool press=down&&!previous;
        previous=down; // A press rejected by a menu/attack cannot replay later.
        if(!press || !allowed || ((on|trig)&L3))return false;
        selected=static_cast<Stance>((unsigned(selected)+1)%3);
        return true;
    }
};
}
