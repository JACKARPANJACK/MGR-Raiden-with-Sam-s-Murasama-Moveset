#pragma once
#include <cmath>
#include "SaiMovesetPolicy.h"
namespace SaiPullPolicy {
    constexpr unsigned LeftStickButton = 0x1000;
    inline bool Press(bool key, bool previousKey, unsigned on, unsigned trig, unsigned previousPad) {
        return (key && !previousKey) || ((on | trig) & LeftStickButton & ~previousPad) != 0;
    }
    constexpr unsigned Duration = 36;
    constexpr float Range = 15.0f, StopDistance = 1.2f, Speed = 0.8f;
    inline bool Eligible(unsigned id) {
        switch (id) {
        case 0x20010: case 0x20140: case 0x20142: case 0x20144:
        case 0x2014A: case 0x20150: case 0x20152: case 0x20160: return true;
        default: return false; // em0170 is the hammer/heavy cyborg
        }
    }
    inline float Step(float distance) {
        return distance > StopDistance ? (distance - StopDistance < Speed ? distance - StopDistance : Speed) : 0;
    }
    inline bool SaiAttack(unsigned state) {
        return SaiMovesetPolicy::GroundAttack(state);
    }
    inline bool Safe(unsigned state, bool alive, bool air, bool blade, bool scripted, bool pulling = false) {
        // Pull injects a native Sai heavy attack. Keep the victim until the
        // pull finishes through that attack and its grounded recovery states.
        return alive && (!air || (pulling && SaiAttack(state))) && !blade && !scripted &&
            ((state <= 0x1Au && state != 7 && state != 0xEu) ||
             (state>=0x100000 && state<0x100040) || (pulling && SaiAttack(state)));
    }
}
