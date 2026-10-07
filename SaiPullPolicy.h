#pragma once
#include <cmath>
namespace SaiPullPolicy {
    constexpr unsigned Duration = 36, Cooldown = 90;
    constexpr float Range = 15.0f, StopDistance = 1.5f, Speed = 0.45f;
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
    inline bool Safe(unsigned state, bool alive, bool air, bool blade, bool scripted) {
        return alive && !air && !blade && !scripted && state <= 0x1Au && state != 7 && state != 0xEu;
    }
}
