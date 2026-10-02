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
    inline int heldLight[] = {-1, 5, 3, 6, 4};
    inline int heldHeavy[] = {-1, 1, 0, 2, 12};
    inline int flickLight[] = {-1, 5, 10, 6, 11};
    inline int flickHeavy[] = {-1, 7, 12, 2, 4};
    inline int Move(Direction direction, bool heavy, bool flick)
    {
        if (direction == None) return -1;
        return (flick ? (heavy ? flickHeavy : flickLight) :
                        (heavy ? heldHeavy : heldLight))[direction];
    }
    struct Input
    {
        Direction held = None, flick = None;
        unsigned flickTicks = 0;
        bool centered = true;
        float accumAngle = 0.0f;
        float prevAngle = 0.0f;
        bool hasPrevAngle = false;
        unsigned flick360Ticks = 0;
        unsigned neutralTicks = 0;

        void Reset() { *this = {}; }

        int Poll(float x, float y, bool light, bool heavy)
        {
            held = Resolve(x, y);
            const float r = std::sqrt(x * x + y * y);
            const bool neutral = r < 0.25f;

            if (flickTicks && --flickTicks == 0) flick = None;
            if (flick360Ticks && --flick360Ticks == 0) {}

            if (neutral)
            {
                centered = true;
                if (++neutralTicks > 15)
                {
                    hasPrevAngle = false;
                    accumAngle = 0.0f;
                }
            }
            else
            {
                neutralTicks = 0;
                if (r >= 0.45f)
                {
                    const float angle = std::atan2(y, x);
                    if (hasPrevAngle)
                    {
                        float delta = angle - prevAngle;
                        constexpr float pi = 3.14159265358979323846f;
                        while (delta > pi) delta -= 2.0f * pi;
                        while (delta < -pi) delta += 2.0f * pi;
                        if (accumAngle * delta < 0.0f && std::fabs(delta) > 0.8f)
                        {
                            accumAngle = delta;
                        }
                        else
                        {
                            accumAngle += delta;
                        }
                        if (std::fabs(accumAngle) >= 1.5f * pi)
                        {
                            flick360Ticks = 30;
                            accumAngle = 0.0f;
                        }
                    }
                    prevAngle = angle;
                    hasPrevAngle = true;
                }
            }

            if (centered && held != None)
            {
                flick = held;
                flickTicks = 8;
                centered = false;
            }

            if (light == heavy) return -1; // simultaneous buttons retain native dodge

            // 360-degree stick flick + Heavy attack performs Judgement Cut charge (Move 7)
            if (heavy && flick360Ticks > 0)
            {
                flick360Ticks = 0;
                flickTicks = 0;
                flick = None;
                return 7;
            }

            const int result = Move(flickTicks ? flick : held, heavy, flickTicks != 0);
            if (result >= 0) { flickTicks = 0; flick = None; }
            return result;
        }
    };
}
