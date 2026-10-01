#pragma once

#include <Pl0000.h>
#include <Hw.h>
#include <windows.h>
#include <cstring>
#include "SamResourceManager.h"
#include "SamSheathPolicy.h"

class SheathController
{
private:
    bool m_bAttachedToHip = false;
    Pl0000* m_pLastPlayer = nullptr;
    char m_lastAnimCode[16]{};
    int m_lastSwordHidden = -1;
    Entity* m_boundSheath = nullptr;
    bool m_savedConstraint = false;
    unsigned m_savedBone = 0x711, m_savedRotationBone = 0;
    Hw::cVec4 m_savedOffset{}, m_savedRotation{};

    static void SanitizeCode(char* dst, size_t cap, const char* src)
    {
        if (!dst || cap == 0) return;
        dst[0] = '\0';
        if (!src || !*src) return;

        if (cap >= 5) SamSheathPolicy::Code(dst, src);
    }

    void RepositionSheathToHip(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            Entity* pSheath = player->m_pSheathEntity;
            if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000)
                return;

            // Make sure sheath is visible
            if (pSheath->m_pBehavior)
                pSheath->m_pBehavior->onDisp();

            // Re-bind Constraint 3 (sheath) to left hip bone (0x710) using Sam's native logic
            Constraints* c = player->getConstraintById(3);
            if (m_boundSheath != pSheath)
            {
                m_boundSheath = pSheath;
                m_lastAnimCode[0] = 0;
                m_lastSwordHidden = -1;
            }
            if (!m_savedConstraint && c)
            {
                m_savedConstraint = true;
                m_savedBone = c->m_nBone;
                m_savedRotationBone = c->m_nRotationBone;
                m_savedOffset = c->m_vecOffset;
                m_savedRotation = c->m_vecRotation;
            }
            const bool rebind = !c || c->m_nBone != 0x710;
            if (!c || c->m_nBone != 0x710)
            {
                player->removeConstraint(3);
                player->attachObject(3, player->m_pEntity, pSheath, 0x710, 0);
                player->setConstraintsBone(3, 0x710, 0);
                c = player->getConstraintById(3);
            }

            if (c && rebind)
            {
                c->m_nBone = 0x710;
                c->m_nRotationBone = 0;
                // Zero offsets: Sam's Murasama sheath (pl1404) origin is calibrated
                // to sit naturally on hip bone 0x710 without intersecting the body
                c->m_vecOffset = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
                c->m_vecRotation = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void RestoreSheathToBack(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            Entity* pSheath = player->m_pSheathEntity;
            if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000)
                return;

            // Restore the original attachment, including costume-specific transforms.
            Constraints* c = player->getConstraintById(3);
            if (!c || c->m_nBone != (m_savedConstraint ? m_savedBone : 0x711))
            {
                player->removeConstraint(3);
                player->attachObject(3, player->m_pEntity, pSheath, m_savedConstraint ? m_savedBone : 0x711, m_savedConstraint ? m_savedRotationBone : 0);
                player->setConstraintsBone(3, m_savedConstraint ? m_savedBone : 0x711, m_savedConstraint ? m_savedRotationBone : 0);
                c = player->getConstraintById(3);
            }

            if (c)
            {
                c->m_nBone = m_savedConstraint ? m_savedBone : 0x711;
                c->m_nRotationBone = m_savedConstraint ? m_savedRotationBone : 0;
                c->m_vecOffset = m_savedConstraint ? m_savedOffset : Hw::cVec4(0, 0, 0, 0);
                c->m_vecRotation = m_savedConstraint ? m_savedRotation : Hw::cVec4(0, 0, 0, 0);
            }
            m_savedConstraint = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void SyncSheathAnimation(Pl0000* player)
    {
        if (!player) return;
        Entity* pSheath = player->m_pSheathEntity;
        if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000 || !pSheath->m_pBehavior)
            return;

        const char* animName = nullptr;
        if (player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray)
            animName = player->m_pAnimationSlot->m_pArray[0].m_pAnimName;

        char cleanCode[16]{};
        if (animName && animName[0] != '\0')
            SanitizeCode(cleanCode, sizeof(cleanCode), animName);
        else
            std::strcpy(cleanCode, "0000");

        const bool animChanged = (std::strcmp(cleanCode, m_lastAnimCode) != 0);
        const bool swordChanged = (player->m_bSwordHidden != m_lastSwordHidden);

        if (animChanged || swordChanged)
        {
            std::strncpy(m_lastAnimCode, cleanCode, sizeof(m_lastAnimCode) - 1);
            m_lastSwordHidden = player->m_bSwordHidden;

            void* sheathMot = SamResourceManager::Instance().GetSheathMotion(cleanCode);
            if (sheathMot)
            {
                pSheath->m_pBehavior->setDirectAnimation(
                    sheathMot, nullptr, 0, 0.05f, 1.0f, 0x8000000, 0.0f, 1.0f);
            }
            else
            {
                // Fallback to sheath map state if no specific motion matched
                pSheath->m_pBehavior->requestAnimationByMap(player->m_bSwordHidden ? 4 : 5);
            }
        }
    }

public:
    static SheathController& Instance()
    {
        static SheathController s_instance;
        return s_instance;
    }

    SheathController() = default;

    void SetSheathToHip(Pl0000* player, bool toHip)
    {
        if (!player) return;

        if (m_pLastPlayer != player)
        {
            m_pLastPlayer = player;
            m_bAttachedToHip = false;
            m_lastAnimCode[0] = '\0';
            m_lastSwordHidden = -1;
            m_boundSheath = nullptr;
            m_savedConstraint = false;
        }

        m_bAttachedToHip = toHip;

        if (toHip)
        {
            RepositionSheathToHip(player);
            SyncSheathAnimation(player);
        }
        else
        {
            RestoreSheathToBack(player);
            m_lastAnimCode[0] = '\0';
            m_lastSwordHidden = -1;
            m_boundSheath = nullptr;
            m_savedConstraint = false;
        }
    }

    void Update(Pl0000* player)
    {
        if (!player) return;

        if (m_pLastPlayer != player)
        {
            m_pLastPlayer = player;
            m_bAttachedToHip = false;
            m_lastAnimCode[0] = '\0';
            m_lastSwordHidden = -1;
        }

        if (!m_bAttachedToHip)
            return;

        RepositionSheathToHip(player);
        SyncSheathAnimation(player);
    }

    void AnimateSheathDirect(Pl0000* player, const char* code)
    {
        if (!player || !code) return;
        Entity* pSheath = player->m_pSheathEntity;
        if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000 || !pSheath->m_pBehavior)
            return;

        m_boundSheath = pSheath;
        char cleanCode[16]{};
        SanitizeCode(cleanCode, sizeof(cleanCode), code);
        void* sheathMot = SamResourceManager::Instance().GetSheathMotion(cleanCode);
        if (sheathMot)
        {
            std::strncpy(m_lastAnimCode, cleanCode, sizeof(m_lastAnimCode) - 1);
            m_lastSwordHidden = player->m_bSwordHidden;
            pSheath->m_pBehavior->setDirectAnimation(
                sheathMot, nullptr, 0, 0.05f, 1.0f, 0x8000000, 0.0f, 1.0f);
        }
    }

    bool IsAttachedToHip() const { return m_bAttachedToHip; }
};
