#pragma once
#include <cstring>
#include "SamArchiveLookup.h"
namespace SamSheathPolicy
{
    inline bool Code(char* dst, const char* name)
    {
        if (!dst) return false;
        dst[0] = 0;
        if (!name) return false;
        const char* p = name;
        if (std::strlen(p) >= 7 && p[6] == '_') p += 7;
        if (std::strlen(p) < 4) return false;
        std::memcpy(dst, p, 4); dst[4] = 0;
        if (!SamArchiveLookup::ValidCode(dst)) { dst[0] = 0; return false; }
        return true;
    }
    inline const char* MotionCode(const char* code)
    {
        // Sam's player charge clips use a separate scabbard numbering scheme.
        struct Pair { const char* actor; const char* sheath; };
        constexpr Pair pairs[] = {
            {"2100", "3010"}, {"2030", "3010"}, {"2102", "3016"},
            {"2101", "3017"}, {"2032", "3015"}, {"2038", "3014"},
            {"2040", "3020"}, {"2041", "3022"}, {"2042", "3024"},
            {"2048", "3024"}
        };
        for (const auto& pair : pairs) if (!std::strcmp(code, pair.actor)) return pair.sheath;
        return code;
    }
}
