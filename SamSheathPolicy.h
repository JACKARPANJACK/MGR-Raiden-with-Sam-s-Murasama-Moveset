#pragma once
#include <cstring>
#include "SamArchiveLookup.h"
namespace SamSheathPolicy
{
    // Native RVA 0x478EA0-0x478EBE pushes arguments right-to-left:
    // rotation 10, bone 0x701, sheath, player entity, constraint 17.
    // Attach the sheath to the player's hand; never constrain the body to it.
    template<class Actor, class Entity>
    inline void BindGrip(Actor* player, Entity* sheath)
    {
        player->removeConstraint(17);
        player->attachObject(17, player->m_pEntity, sheath, 0x701, 10);
        auto* grip = player->getConstraintById(17);
        if (grip) {
            grip->m_vecOffset = decltype(grip->m_vecOffset){0, 0, 0, 0};
            grip->m_vecRotation = decltype(grip->m_vecRotation){0, 0, 0, 0};
        }
    }
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
        // Native DLC player/scabbard maps use identical IDs and clip codes.
        // Only imported boss charge stages need a playable scabbard fallback.
        struct Pair { const char* actor; const char* sheath; };
        constexpr Pair pairs[] = {
            {"3000", "2250"}, {"3010", "2250"}, {"3020", "2250"}, {"92e0", "2250"},
            {"3003", "2253"}, {"3004", "2253"}, {"3014", "2253"}, {"3024", "2253"}, {"3015", "2253"},
            {"92e4", "2253"}
        };
        for (const auto& pair : pairs) if (!std::strcmp(code, pair.actor)) return pair.sheath;
        return code;
    }
}
