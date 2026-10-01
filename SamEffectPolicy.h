#pragma once
#include <cstdint>
namespace SamEffectPolicy
{
    // Translate only charge helpers to IDs verified in pl1400.eff. Other
    // shared helpers must keep their native 10010 bank and effect numbers.
    inline int Remap(int id, uintptr_t caller, bool active)
    {
        if (!active) return id;
        if (caller >= 0x7C7900 && caller < 0x7C7E00)
        {
            if (id == 333) return 117;
            if (id == 334) return 118;
            if (id == 335) return 119;
        }
        if (caller >= 0x7F7100 && caller < 0x7F7300 && id == 181) return 121;
        return id;
    }
}
