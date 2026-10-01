#pragma once
#include <cmath>
#include <algorithm>
namespace SamRoundTripPolicy
{
    constexpr unsigned ReleaseFrame = 12;
    constexpr unsigned ReturnTick = 130;
    constexpr unsigned TimeoutTick = 260;
    inline bool CanHit(float x, float y, float z, bool alive)
    {
        return alive && x*x + y*y + z*z <= 2.5f*2.5f;
    }
    inline void Step(float& x, float& y, float& z,
        float tx, float ty, float tz, float speed)
    {
        const float dx = tx-x, dy = ty-y, dz = tz-z;
        const float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (distance <= 0.0001f) return;
        const float fraction = (std::min)(distance, speed) / distance;
        x += dx*fraction; y += dy*fraction; z += dz*fraction;
    }
}
