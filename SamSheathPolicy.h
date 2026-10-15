#pragma once
#include <cstring>
#include <cmath>
#include "SamArchiveLookup.h"
namespace SamSheathPolicy
{
    // Native draw-state marker at RVA 0x795577..0x795610. Keep its
    // hysteresis and all four hand/foot mounts, including the return to zero.
    inline int SwordState(float x,float y,int previous) {
        if(!std::isfinite(x)||!std::isfinite(y))return previous;
        int state=previous;
        if(std::fabs(x)<.01f && std::fabs(y)<.01f)state=0;
        if(x<-.09f)state=1;
        if(x>.09f)state=2;
        if(y>.09f)state=3;
        if(y<-.09f)state=4;
        return state;
    }
    inline unsigned SwordSocket(unsigned sheathId) {return sheathId==0x11404u?0x710u:0x700u;}
    template<class Actor,class Entity>
    inline bool BindParkedSword(Actor* player,Entity* sheath,Entity* sword,unsigned socket) {
        if(!player || !sheath || !sheath->m_pBehavior || !sword)return false;
        auto* actor=sheath->m_pBehavior;
        auto* mount=actor->getConstraintById(4);
        if(!player->getConstraintById(4) && mount && mount->m_MainEntity.getEntity()==sheath &&
            mount->m_ObjectEntityHandle.getEntity()==sword && mount->m_nBone==socket && mount->m_nRotationBone==0 &&
            mount->m_vecOffset.x==0 && mount->m_vecOffset.y==0 && mount->m_vecOffset.z==0 &&
            mount->m_vecRotation.x==0 && mount->m_vecRotation.y==0 && mount->m_vecRotation.z==0)return false;
        // Exactly one owner: park on the sheath actor, never a stale player
        // constraint whose parent entity merely happens to be the sheath.
        player->removeConstraint(4);actor->removeConstraint(4);
        actor->attachObject(4,sheath,sword,socket,0);
        if(auto* c=actor->getConstraintById(4)) {
            c->m_vecOffset=decltype(c->m_vecOffset){0,0,0,0};
            c->m_vecRotation=decltype(c->m_vecRotation){0,0,0,0};
        }
        return true;
    }
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
