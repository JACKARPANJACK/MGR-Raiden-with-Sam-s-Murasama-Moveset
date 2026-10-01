#pragma once
#include <cstdint>

namespace SamTogglePolicy
{
    inline bool CanActivate(uint32_t action, bool alive)
    {
        if (!alive) return false;
        switch (action)
        {
        case 0: case 1: case 2: case 3: case 4: // idle / locomotion (walk, run, turn)
        case 9: case 0xB: case 0xC: // jump / fall / landing
        case 0x4F: case 0x50: case 0x51: case 0x53:
        case 0x5B: case 0x5C: case 0x5D: case 0x6B: case 0x6C:
            return true;
        default: return false; // wait for native damage / scripted states
        }
    }
    inline uint32_t EntryState(bool airborne) { return airborne ? 0x100005u : 0x100000u; }
    inline uint32_t ExitState(bool airborne) { return airborne ? 0xBu : 0u; }
    inline bool OwnsAction(uint32_t action) { return action >= 0x100000u && action < 0x100080u; }
}
