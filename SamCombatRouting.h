#pragma once
#include <cstring>
#include "SamAnimationPairs.h"
#include "SamArchiveLookup.h"

namespace SamCombatRouting
{
    inline bool IsNativeAirTransition(const char* code)
    {
        return SamArchiveLookup::ValidCode(code) &&
            (std::strncmp(code, "023", 3) == 0 || std::strncmp(code, "024", 3) == 0);
    }
    inline bool IsMotion(const char* name)
    {
        const char* suffix = name ? std::strrchr(name, '.') : nullptr;
        return suffix && std::strcmp(suffix, ".mot") == 0;
    }

    inline bool IsCombatCode(const char* code)
    {
        if (!code || std::strlen(code) < 4) return false;
        if (code[0] != '2' && std::strncmp(code, "350", 3) != 0) return false;
        for (int i = 1; i < 4; ++i)
            if (!((code[i] >= '0' && code[i] <= '9') ||
                  (code[i] >= 'a' && code[i] <= 'f'))) return false;
        return true;
    }

    // Raiden and Sam do not share a combat animation numbering scheme.
    // These are Raiden request codes observed in live gameplay, mapped to Sam's
    // DLC (pl1400) and Boss (em0020) combat animations.
    inline const char* SamCodeForRaiden(const char* code)
    {
        struct Pair { const char* raiden; const char* sam; };
        static constexpr Pair pairs[] = {
            // Stormbringer (360 spin + heavy) -> Judgement Cut
            {"3500", "92e0"}, {"3501", "92e4"}, {"3502", "92e4"},

            // Ground Light Attack String & branches
            {"2100", "2000"}, {"2101", "2001"}, {"2102", "2002"}, {"2103", "2003"},
            {"2108", "2003"},
            {"2110", "2110"}, {"2111", "2111"}, {"2112", "2112"},
            {"2120", "2120"}, // Dash thrust / stinger
            {"2121", "2120"}, // Forward-Forward light / thrust
            {"2122", "2001"}, {"2123", "2002"}, {"2124", "2002"},
            {"2125", "2003"}, {"2126", "2003"}, {"212d", "2002"},
            {"2130", "2003"}, {"2131", "2003"},
            {"2140", "2510"}, {"2141", "2520"}, {"2142", "2530"}, {"2143", "2540"}, // Directional light dodges
            {"2150", "2410"}, {"2151", "2411"}, {"2152", "2412"}, // Back-Forward launcher combo
            {"2153", "2410"}, {"2154", "2411"}, {"2155", "2412"}, {"2156", "2412"},
            {"2157", "2031"}, {"2158", "2032"}, // Blade flurry -> Sam multi-slash

            // Ground Heavy Attack String 1 (Launcher chain)
            {"2160", "2020"}, {"2161", "2021"}, {"2162", "2022"}, {"2163", "2028"},
            {"2165", "2021"}, {"2166", "2022"}, {"216f", "2028"},

            // Ground Heavy Attack String 2 (Sweep / Branch chain)
            {"2170", "2030"}, {"2171", "2031"},
            {"2172", "2600"}, {"2173", "2610"}, {"2174", "2600"}, {"2176", "2610"}, // Stick flick / spin slashes
            {"2175", "2038"},
            {"2177", "2032"}, {"2178", "2038"}, {"217a", "2038"}, {"217f", "2038"},

            // Ground Heavy Attack String 3
            {"2180", "2040"}, {"2181", "2041"}, {"2182", "2042"},

            // Sprint / Dodge / Stance Attacks / Offensive Defense
            {"2200", "2200"}, {"2203", "2003"},
            {"2208", "2200"}, {"2209", "2020"}, {"220c", "2120"},
            {"2240", "2510"}, {"2241", "2520"}, {"2242", "2530"}, {"2243", "2540"}, // Directional heavy dodges

            // Offensive Defense / Evade slashes
            {"2300", "2510"}, {"2310", "2510"}, {"2313", "2510"}, {"2314", "2510"},
            {"2320", "2530"}, {"2323", "2530"}, {"2324", "2530"}, {"2325", "2530"},
            {"2330", "2540"}, {"2333", "2540"}, {"2334", "2540"}, {"2335", "2540"},
            {"2350", "2520"},

            // Charge Attacks
            {"2250", "2250"}, {"2251", "2251"}, {"2252", "2252"}, {"2253", "2253"},
            {"2255", "2253"},

            // Raiden Heavy String (2260-2262) -> Sam Heavy String (2020-2022)
            {"2260", "2020"}, {"2261", "2021"}, {"2262", "2022"},

            // Sam Heavy Combo (2360, 2370-2375)
            {"2360", "2200"},
            {"2370", "2210"}, {"2371", "2211"}, {"2372", "2211"},
            {"2373", "2212"}, {"2374", "2218"}, {"2375", "2218"},

            // Aerial Attacks
            {"2400", "2400"}, {"2408", "2408"},
            {"2410", "2400"},
            {"2420", "2410"}, {"2421", "2411"}, {"2422", "2412"},
            {"2430", "2410"}, {"2431", "2411"}, {"2432", "2412"},
            {"24c0", "24c0"}, {"24c1", "24c1"}, {"24c2", "24c2"},

            // Boss Sam Special Attacks (em0020 fallback)
            {"2500", "2500"}, {"2510", "2510"}, {"2520", "2520"},
            {"2522", "2522"}, {"2530", "2530"}, {"2532", "2532"},
            {"2540", "2540"}, {"2542", "2542"},
            {"2600", "2600"}, {"2601", "2600"}, {"2602", "2610"}, {"2610", "2610"},
            {"2700", "2700"}, {"2800", "2800"}
        };
        for (const auto& pair : pairs)
            if (std::strcmp(code, pair.raiden) == 0) return pair.sam;

        // Intelligent category fallback for any unmapped 2xxx combat codes to ensure
        // a Sam combat animation is ALWAYS played instead of native Raiden moves:
        if (IsCombatCode(code))
        {
            switch (code[1])
            {
            case '1': return "2000"; // Light chain default -> Sam Light 1
            case '2': return "2020"; // Heavy chain default -> Sam Heavy 1
            case '3': return "2210"; // Combo heavy default -> Sam Heavy combo 1
            case '4': return "2400"; // Air attack default -> Sam Air Light 1
            case '5': case '6': case '7': case '8': return code; // Special / boss codes
            default: break;
            }
        }
        return code;
    }

    // Native Animation::findMotion receives a code; GetSequenceFile receives
    // an actor-prefixed stem. Both must select the same combat pair.
    inline const char* CodeFromRequest(const char* name)
    {
        if (!name) return nullptr;
        const char* code = name;
        if (std::strncmp(name, "pl0000_", 7) == 0 ||
            std::strncmp(name, "pl0010_", 7) == 0)
            code += 7;
        if (std::strlen(code) != 4 || !IsCombatCode(code)) return nullptr;
        return code;
    }

    inline const char* CodeFromFile(const char* name)
    {
        if (!name || (std::strncmp(name, "pl0000_", 7) != 0 &&
            std::strncmp(name, "pl0010_", 7) != 0))
            return nullptr;
        const char* code = name + 7;
        const size_t length = std::strlen(code);
        const bool motion = length == 8 && std::strcmp(code + 4, ".mot") == 0;
        const bool sequence = length == 14 && code[4] == '_' &&
            code[5] >= '0' && code[5] <= '9' && std::strcmp(code + 6, "_seq.bxm") == 0;
        if (!motion && !sequence) return nullptr;
        // Keep locomotion, hit reactions, executions and other character-specific
        // transitions native. Only known combat clips are eligible.
        return IsCombatCode(code) ? code : nullptr;
    }
    struct RequestClip
    {
        char code[5]{};
        char samCode[5]{};
        SamArchiveLookup::Clip clip{};

        template<class Resolver>
        bool Select(const char* name, Resolver resolve)
        {
            std::memset(code, 0, sizeof(code));
            std::memset(samCode, 0, sizeof(samCode));
            clip = {};
            const char* combatCode = CodeFromRequest(name);
            if (!combatCode) return false;
            std::memcpy(code, combatCode, 4);
            std::memcpy(samCode, SamCodeForRaiden(code), 4);
            clip = resolve(samCode);
            return clip.motion && clip.sequence;
        }

        void* SequenceFor(const char* name) const
        {
            const char* combatCode = CodeFromRequest(name);
            return combatCode && std::memcmp(combatCode, code, 4) == 0 &&
                clip.motion && clip.sequence ? clip.sequence : nullptr;
        }
    };

}
