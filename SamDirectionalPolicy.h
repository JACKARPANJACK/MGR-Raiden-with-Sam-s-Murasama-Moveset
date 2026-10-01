#pragma once
#include <cmath>

namespace SamDirectionalPolicy
{
    enum Direction { None, Forward, Back, Left, Right };
    inline Direction Resolve(float x, float y)
    {
        if (std::fabs(x) < 0.55f && std::fabs(y) < 0.55f) return None;
        if (std::fabs(y) >= std::fabs(x)) return y > 0 ? Forward : Back;
        return x < 0 ? Left : Right;
    }
    // Indices into the shared stage catalog, excluded from the X ultimate set.
    inline int Move(Direction direction, bool heavy, bool flick)
    {
        if (direction == None) return -1;
        constexpr int heldLight[] = {-1, 5, 3, 6, 4};
        constexpr int heldHeavy[] = {-1, 1, 0, 2, 12};
        constexpr int flickLight[] = {-1, 5, 10, 6, 11};
        constexpr int flickHeavy[] = {-1, 7, 12, 2, 4};
        return (flick ? (heavy ? flickHeavy : flickLight) :
                        (heavy ? heldHeavy : heldLight))[direction];
    }
    struct Input
    {
        Direction held = None, flick = None;
        unsigned flickTicks = 0;
        bool centered = true;
        void Reset() { *this = {}; }
        int Poll(float x, float y, bool light, bool heavy)
        {
            held = Resolve(x, y);
            const bool neutral = std::fabs(x) < 0.25f && std::fabs(y) < 0.25f;
            if (flickTicks && --flickTicks == 0) flick = None;
            if (neutral) centered = true;
            if (centered && held != None)
            {
                flick = held;
                flickTicks = 8;
                centered = false;
            }
            if (light == heavy) return -1; // simultaneous buttons retain native dodge
            const int result = Move(flickTicks ? flick : held, heavy, flickTicks != 0);
            if (result >= 0) { flickTicks = 0; flick = None; }
            return result;
        }
    };
}
