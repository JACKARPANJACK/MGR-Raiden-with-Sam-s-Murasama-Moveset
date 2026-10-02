#pragma once

#include <Pl0000.h>
#include <EntitySystem.h>
#include <Hw.h>
#include <windows.h>
#include <cstring>
#include "SamResourceManager.h"
#include "SamSheathPolicy.h"

class SheathController
{
private:
    bool m_bAttachedToHip = false;
    bool m_bAttachedToHand = false;
    Pl0000* m_pLastPlayer = nullptr;
    char m_lastAnimCode[16]{};
    int m_lastSwordHidden = -1;
    Entity* m_boundSheath = nullptr;
    bool m_savedConstraint = false;
    unsigned m_savedBone = 0x711, m_savedRotationBone = 0;
    Hw::cVec4 m_savedOffset{}, m_savedRotation{};
    EntityHandle m_originalSheath{};
    EntityHandle m_samSheath{};
    EntityHandle m_modelSwapSheath{};
    eObjID m_modelSwapOriginalObject{};
    eObjID m_modelSwapOriginal{};
    bool m_modelSwapped = false;
    bool m_samDetached = false;
    unsigned m_sheathTracePlayer = 0;
    bool m_sheathTraceDone = false;

    static void Trace(const char* text)
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        char* slash = std::strrchr(path, '\\');
        if (slash) std::strcpy(slash + 1, "scripts\\RaidenMoveset-debug.log");
        FILE* f = nullptr;
        if (fopen_s(&f, path, "a") == 0 && f)
        {
            std::fprintf(f, "[SamMoveset] SHEATH %s\n", text ? text : "unknown");
            std::fclose(f);
        }
    }

    static void PrepareNativeSheath(Entity* entity)
    {
        if (!entity || !entity->m_pBehavior) return;
        // IDA: Pl0000's native sheath path calls sub_611C60(behavior, 0)
        // immediately after attachObject(3,...). This builds the Pl0013
        // scene/animation state; createEntity alone leaves a detached model.
        using Prepare = int(__thiscall*)(Behavior*, int);
        reinterpret_cast<Prepare>(shared::base + 0x611C60)(entity->m_pBehavior, 0);
        // IDA 0x7BD787-0x7BD792: the native path then invokes behavior
        // vtable slot 25 (vtable byte offset 0x64) before setting m_pOwner.
        using NativeFinalize = void(__thiscall*)(Behavior*);
        NativeFinalize* vtable = *reinterpret_cast<NativeFinalize**>(entity->m_pBehavior);
        if (vtable && vtable[0x64 / sizeof(void*)])
            vtable[0x64 / sizeof(void*)](entity->m_pBehavior);
    }

    static unsigned NativeSamSheathBone(Pl0000* player)
    {
        // Sam native sheath logic (IDA RVA 0x46ECA4):
        // Sam's scabbard attaches to dedicated bone 0x7F0 with zero offset & rotation.
        // Merged player models contain bone 0x7F0 with Sam's exact rest transform.
        if (player && player->getPartsPtr(0x7F0u))
            return 0x7F0u;

        // Fallback for custom skins missing 0x7F0:
        const auto mode = *reinterpret_cast<const unsigned*>(shared::base + 0x1BEA024);
        return 0x710u + (mode == 0xEu ? 1u : 0u);
    }

    static unsigned NativeRaidenSheathBone()
    {
        // Raiden native sheath logic (IDA 0x7BD7BE-0x7BD7D3):
        // Selects 0x710, or 0x711 when the player mode global is 0xE.
        const auto mode = *reinterpret_cast<const unsigned*>(shared::base + 0x1BEA024);
        return 0x710u + (mode == 0xEu ? 1u : 0u);
    }

    bool UseSamSheath(Pl0000* player)
    {
        if (m_sheathTracePlayer != reinterpret_cast<unsigned>(player))
        {
            m_sheathTracePlayer = reinterpret_cast<unsigned>(player);
            m_sheathTraceDone = false;
        }
        Entity* sam = m_samSheath.getEntity();
        if (sam && player->m_pSheathEntity == sam) return true;
        if (sam && m_samDetached)
        {
            Entity* original = player->m_pSheathEntity;
            if (!original || !original->m_pBehavior) return false;
            Constraints* c = player->getConstraintById(3);
            if (c && !m_savedConstraint)
            {
                m_savedConstraint = true;
                m_savedBone = c->m_nBone;
                m_savedRotationBone = c->m_nRotationBone;
                m_savedOffset = c->m_vecOffset;
                m_savedRotation = c->m_vecRotation;
            }
            m_originalSheath = original;
            original->m_pBehavior->removeConstraint(4);
            original->m_pBehavior->offDisp();
            player->removeConstraint(3);
            const unsigned samBone = NativeSamSheathBone(player);
            player->attachObject(3, player->m_pEntity, sam, samBone, 0);
            c = player->getConstraintById(3);
            if (c)
            {
                c->m_vecOffset = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
                c->m_vecRotation = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
            PrepareNativeSheath(sam);
            sam->m_pBehavior->m_pOwner = player;
            sam->m_pBehavior->onDisp();
            player->m_SheathHandle = sam;
            player->m_pSheathEntity = sam;
            m_samDetached = false;
            m_lastAnimCode[0] = 0;
            m_lastSwordHidden = -1;
            Trace("reused streamed Sam sheath carrier");
            return true;
        }
        if (sam)
        {
            // Keep a carrier alive across the toggle. Releasing a cObj whose
            // data file was rebound to the streamed DLC archive runs the
            // engine's archive destructor while the archive manager still
            // owns it and tears down the game process.
            sam->m_pBehavior->offDisp();
            m_samSheath = nullptr;
            m_originalSheath = nullptr;
            m_savedConstraint = false;
            m_samDetached = false;
        }
        Entity* original = player->m_pSheathEntity;
        if (!original || !original->m_pBehavior)
        {
            if (!m_sheathTraceDone) { Trace("original sheath unavailable"); m_sheathTraceDone = true; }
            return false;
        }
        // The resource manager mounts data107.cpk and retains pl1404 until
        // player shutdown. Never construct an entity from an unfinished stream.
        if (!g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x11404), 0))
        {
            if (!m_sheathTraceDone) { Trace("pl1404 not loaded yet"); m_sheathTraceDone = true; }
            return false;
        }

        // IDA 0x18a0b00: entity factory registers 0x11404 with class name "Pl0013",
        // but in Raiden's campaign, Pl0013 is hardcoded to the Prologue Sheath.
        sam = g_EntitySystem.createEntity("Pl0013", static_cast<eObjID>(0x11404), nullptr);
        if (!sam)
        {
            if (!m_sheathTraceDone)
            {
                Trace("createEntity failed for Sam pl1404 sheath (0x11404)");
                m_sheathTraceDone = true;
            }
            return false;
        }
        if (!sam->m_pBehavior) { Trace("created entity has no behavior"); sam->release(); return false; }
        Constraints* c = player->getConstraintById(3);
        if (c && !m_savedConstraint)
        {
            m_savedConstraint = true;
            m_savedBone = c->m_nBone;
            m_savedRotationBone = c->m_nRotationBone;
            m_savedOffset = c->m_vecOffset;
            m_savedRotation = c->m_vecRotation;
        }
        m_originalSheath = original;
        m_samSheath = sam;
        m_samDetached = false;
        // Match the native order from IDA: attach, initialize scene state,
        // then assign the player owner used by bone-space updates.
        original->m_pBehavior->removeConstraint(4);
        original->m_pBehavior->offDisp();
        player->removeConstraint(3);
        const unsigned samBone = NativeSamSheathBone(player);
        player->attachObject(3, player->m_pEntity, sam, samBone, 0);
        c = player->getConstraintById(3);
        if (c)
        {
            c->m_nBone = samBone;
            c->m_nRotationBone = 0;
            if (samBone == 0x7F0u)
            {
                c->m_vecOffset = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
                c->m_vecRotation = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                c->m_vecOffset = Hw::cVec4(-0.10f, 0.02f, -0.04f, 0.0f);
                c->m_vecRotation = Hw::cVec4(-0.15f, 0.0f, 0.10f, 0.0f);
            }
        }
        PrepareNativeSheath(sam);
        sam->m_pBehavior->m_pOwner = player;
        sam->m_pBehavior->onDisp();
        player->m_SheathHandle = sam;
        player->m_pSheathEntity = sam;
        Trace("Sam pl1404 sheath attached");
        m_sheathTraceDone = false;
        m_lastAnimCode[0] = 0;
        m_lastSwordHidden = -1;
        return true;
    }

    void UseOriginalSheath(Pl0000* player)
    {
        Entity* sam = m_samSheath.getEntity();
        Entity* original = m_originalSheath.getEntity();
        if (m_bAttachedToHand)
        {
            player->removeConstraint(17);
            m_bAttachedToHand = false;
        }
        if (sam && player->m_pSheathEntity == sam)
        {
            sam->m_pBehavior->removeConstraint(4);
            sam->m_pBehavior->offDisp();
            player->removeConstraint(3);
            player->m_SheathHandle = original;
            player->m_pSheathEntity = original;
            if (original && original->m_pBehavior) original->m_pBehavior->onDisp();
            m_samDetached = true;
        }
    }

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
            if (!UseSamSheath(player)) return;
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
            const unsigned sheathBone = NativeSamSheathBone(player);
            const bool rebind = !c || c->m_nBone != sheathBone;
            if (rebind)
            {
                player->removeConstraint(3);
                player->attachObject(3, player->m_pEntity, pSheath, sheathBone, 0);
                player->setConstraintsBone(3, sheathBone, 0);
                c = player->getConstraintById(3);
            }

            if (c && rebind)
            {
                c->m_nBone = sheathBone;
                c->m_nRotationBone = 0;
                if (sheathBone == 0x7F0u)
                {
                    // Zero offsets: Sam's Murasama sheath (pl1404) origin is calibrated
                    // to sit naturally on hip bone 0x7F0 without intersecting the body
                    c->m_vecOffset = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
                    c->m_vecRotation = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
                }
                else
                {
                    c->m_vecOffset = Hw::cVec4(-0.10f, 0.02f, -0.04f, 0.0f);
                    c->m_vecRotation = Hw::cVec4(-0.15f, 0.0f, 0.10f, 0.0f);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void RestoreSheathToBack(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            if (m_bAttachedToHand)
            {
                player->removeConstraint(17);
                m_bAttachedToHand = false;
            }
            UseOriginalSheath(player);
            Entity* pSheath = player->m_pSheathEntity;
            if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000)
                return;

            const unsigned raidenBone = m_savedConstraint ? m_savedBone : NativeRaidenSheathBone();
            const unsigned raidenRotBone = m_savedConstraint ? m_savedRotationBone : 0;

            // Restore the original attachment, including costume-specific transforms.
            Constraints* c = player->getConstraintById(3);
            if (!c || c->m_nBone != raidenBone)
            {
                player->removeConstraint(3);
                player->attachObject(3, player->m_pEntity, pSheath, raidenBone, raidenRotBone);
                player->setConstraintsBone(3, raidenBone, raidenRotBone);
                c = player->getConstraintById(3);
            }

            if (c)
            {
                c->m_nBone = raidenBone;
                c->m_nRotationBone = raidenRotBone;
                c->m_vecOffset = m_savedConstraint ? m_savedOffset : Hw::cVec4(0, 0, 0, 0);
                c->m_vecRotation = m_savedConstraint ? m_savedRotation : Hw::cVec4(0, 0, 0, 0);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void SyncSheathAnimation(Pl0000* player)
    {
        if (!player) return;
        Entity* pSheath = player->m_pSheathEntity;
        if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000 || !pSheath->m_pBehavior)
            return;
        if (!m_modelSwapped && pSheath != m_samSheath.getEntity()) return;

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
            m_bAttachedToHand = false;
            m_lastAnimCode[0] = '\0';
            m_lastSwordHidden = -1;
            m_boundSheath = nullptr;
            m_savedConstraint = false;
            m_originalSheath = nullptr;
            m_samSheath = nullptr;
            m_modelSwapSheath = nullptr;
            m_modelSwapped = false;
            m_samDetached = false;
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
            m_bAttachedToHand = false;
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
            m_bAttachedToHand = false;
            m_lastAnimCode[0] = '\0';
            m_lastSwordHidden = -1;
            m_boundSheath = nullptr;
            m_savedConstraint = false;
            m_originalSheath = nullptr;
            m_samSheath = nullptr;
            m_modelSwapSheath = nullptr;
            m_modelSwapped = false;
            m_samDetached = false;
        }

        if (!m_bAttachedToHip)
            return;

        RepositionSheathToHip(player);

        // Native Sam dynamic hand constraint logic (IDA RVA 0x478E30 - 0x478EBE):
        // Sequence event flag 21 (0x15) signals Sam grasping the sheath with left hand.
        // When active: attach constraint 17 to left hand bone 0x701 with rotation bone 10 (0xA).
        // When inactive: remove constraint 17; sheath rests on hip via constraint 3 (bone 0x7F0).
        Entity* sam = m_samSheath.getEntity();
        if (sam && player->m_pSheathEntity == sam)
        {
            const bool holdInHand = player->ckSeqFlag(21) != 0;
            if (holdInHand && !m_bAttachedToHand)
            {
                player->removeConstraint(17);
                player->attachObject(17, player->m_pEntity, sam, 0x701, 10);
                m_bAttachedToHand = true;
            }
            else if (!holdInHand && m_bAttachedToHand)
            {
                player->removeConstraint(17);
                m_bAttachedToHand = false;
            }
        }

        SyncSheathAnimation(player);
    }

    void AnimateSheathDirect(Pl0000* player, const char* code)
    {
        if (!player || !code) return;
        Entity* pSheath = player->m_pSheathEntity;
        if (!pSheath || reinterpret_cast<uintptr_t>(pSheath) < 0x10000 || !pSheath->m_pBehavior)
            return;
        if (pSheath != m_samSheath.getEntity()) return;

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
