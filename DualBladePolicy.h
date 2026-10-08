#pragma once
#include <cstring>

namespace DualBladePolicy
{
    constexpr int SpecialWeapon = 6;
    constexpr unsigned WeaponObject = 0x32050; // custom wp2050 resource identity
    constexpr unsigned Object = 0x30050; // Native wp0050 model stream
    // Use Raiden's native custom-weapon constraint slot. Slot 4 owns the
    // primary katana; custom weapons use slot 5 and the opposite hand marker.
    constexpr int Constraint = 5, HandBone = 0x701;
    constexpr int GripBone = 0;
    inline constexpr const char* MovementClips[] = {
        "0220","0221","0222","0223","0210","0212","0213"
    };
    inline constexpr const char* CombatClips[] = {
        "2250","2251","2252","2256","4611","4613","4614","4615","4616",
        "2000","2001","2002","2005","2035","2037","2050","2051","2052","2070","2071","2072","2080"
    };
    inline constexpr const char* SundownerClips[] = {
        "2000","2001","2002","2005","2035","2037",
        "2050","2051","2052","2070","2071","2072","2080"
    };
    inline constexpr const char* Clips[] = {
        "0220","0221","0222","0223","0210","0212","0213",
        "2250","2251","2252","2256","4611","4613","4614","4615","4616",
        "2000","2001","2002","2005","2035","2037","2050","2051","2052","2070","2071","2072","2080"
    };
    inline const char* MovementClip(const char* name)
    {
        if (!name) return nullptr;
        const char* code = name;
        if (!std::strncmp(code,"pl0010_",7)) code += 7;
        for (auto clip : MovementClips) if (!std::strncmp(code,clip,4)) return clip;
        return nullptr;
    }
    inline bool IsMovement(const char* name)
    {
        return MovementClip(name) != nullptr;
    }
    inline bool IsSundownerClip(const char* code)
    {
        if (!code) return false;
        for (auto clip : SundownerClips) if (!std::strncmp(code,clip,4) && code[4]=='\0') return true;
        return false;
    }
    inline const char* Resolve(const char* name, bool allowMovement = true)
    {
        if (!name) return nullptr;
        const char* code = name;
        if (!std::strncmp(code,"pl0010_",7)) code += 7;
        const size_t length = std::strlen(code);
        if (length != 4 && !(length == 8 && !std::strcmp(code+4,".mot"))) return nullptr;
        if (const char* mov = MovementClip(code)) return allowMovement ? mov : nullptr;
        for (auto clip : CombatClips) if (!std::strncmp(code,clip,4)) return clip;
        // Keep Raiden's native combo state machine and cancellation map.
        // Map light and heavy strings to varied dual blade and Sundowner Phase 1 clips.
        struct Pair { const char* from; const char* to; };
        constexpr Pair attacks[] = {
            // Light Attack combo string (agile dual blade slashes)
            {"2100","2250"},{"2101","2251"},{"2102","2252"},
            {"2103","2256"},{"2108","2256"},
            // Dash / Stinger forward attacks
            {"2120","2035"},{"2121","2035"},{"2122","2037"},
            // Launcher / Lift attacks
            {"2150","2001"},{"2151","2037"},{"2152","2080"},
            // Heavy Combo string 1 (Sundowner Phase 1 heavy machete slashes)
            {"2160","2050"},{"2161","2051"},{"2162","2052"},
            {"2163","2080"},{"216f","2080"},
            // Heavy Combo string 2 (Sundowner Phase 1 swinging frenzy multi-slashes)
            {"2170","2070"},{"2171","2071"},{"2172","2072"},
            {"2173","2070"},{"2174","2071"},{"2175","2080"},
            {"2176","2072"},{"2177","2052"},{"2178","2080"},
            {"217a","2080"},{"217f","2080"},
            // Sprint attack (Sundowner step-forward running slash)
            {"2200","2005"},
            // Raw heavy attacks
            {"2260","2050"},{"2261","2051"},{"2262","2052"}
        };
        for (auto pair : attacks) if (!std::strncmp(code,pair.from,4)) return pair.to;
        return nullptr;
    }
}
