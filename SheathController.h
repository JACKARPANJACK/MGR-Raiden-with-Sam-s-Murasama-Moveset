#pragma once

#include <Pl0000.h>
#include <EntitySystem.h>
#include <cObjReadManager.h>
#include <Hw.h>
#include <windows.h>
#include <cstring>
#include "SamResourceManager.h"
#include "SamSheathPolicy.h"
#include "1-GameFileFlagCheck.h"
#include "SamUltimatePolicy.h"
#include "SamTemplateExecutionPolicy.h"

class SheathController
{
private:
    bool m_bAttachedToHip = false;
    bool m_bAttachedToHand = false;
    bool m_swapFaulted = false;
    Pl0000* m_pLastPlayer = nullptr;
    bool m_savedConstraint = false;
    uint32_t m_originalSheathId = 0;
    unsigned m_savedBone = 0x711, m_savedRotationBone = 0;
    Hw::cVec4 m_savedOffset{}, m_savedRotation{};
    EntityHandle m_originalSheath{};
    EntityHandle m_samSheath{};
    char m_directCode[5]{};
    float m_directSpeed=1;
    bool m_chargePose=false;
    bool m_chargeGrip=false;

    static void Trace(const char* message)
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        char* slash = std::strrchr(path, '\\');
        if (!slash) return;
        std::strcpy(slash + 1, "scripts\\RaidenMoveset-debug.log");
        FILE* file = nullptr;
        if (fopen_s(&file, path, "a") == 0 && file) {
            std::fprintf(file, "[SamMoveset] SHEATH %s\n", message);
            std::fclose(file);
        }
    }

    static void PrepareNativeSheath(Entity* entity)
    {
        if (!entity || !entity->m_pBehavior) return;
        // Native Pl0013 scene setup: calls sub_A11C60(behavior, 0)
        using Prepare = int(__thiscall*)(Behavior*, int);
        reinterpret_cast<Prepare>(shared::base + 0x611C60)(entity->m_pBehavior, 0); // 0x400000 + 0x611C60 = 0xA11C60
    }

    static unsigned NativeSamSheathBone(Pl0000* player)
    {
        // Sam native sheath logic (IDA RVA 0x46ECA4):
        // Sam's scabbard attaches to dedicated bone 0x7F0 with zero offset & rotation.
        // Merged player models contain bone 0x7F0 with Sam's exact rest transform.
        if (player && player->getPartsPtr(0x7F0u))
            return 0x7F0u;

        // Fallback for custom/standard Raiden skins missing 0x7F0:
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

    void TrackPlayer(Pl0000* player)
    {
        if (m_pLastPlayer == player) return;
        m_pLastPlayer = player;
        m_bAttachedToHip = false;
        m_bAttachedToHand = false;
        m_swapFaulted = false;
        m_savedConstraint = false;
        m_originalSheathId = 0;
        m_originalSheath = nullptr;
        m_samSheath = nullptr;
        m_directCode[0]=0;m_directSpeed=1;m_chargePose=m_chargeGrip=false;
    }

    bool EnsureSamSheath(Pl0000* player)
    {
        if (!player) return false;
        TrackPlayer(player);
        if (m_swapFaulted) return false;

        Entity* current = player->m_pSheathEntity;
        if (!current || reinterpret_cast<uintptr_t>(current) < 0x10000)
            return false;

        const bool currentIsSam = current->m_ObjId == static_cast<eObjID>(0x11404);
        if (currentIsSam) m_samSheath = current;
        else if (m_originalSheath.getEntity() != current || !m_savedConstraint)
        {
            // Capture the actual costume entity before replacing it. A native
            // costume rebuild supplies a new original and attachment transform.
            m_originalSheath = current;
            m_originalSheathId = static_cast<uint32_t>(current->m_ObjId);
            m_savedConstraint = false;
            Constraints* original = player->getConstraintById(3);
            if (original)
            {
                m_savedConstraint = true;
                m_savedBone = original->m_nBone;
                m_savedRotationBone = original->m_nRotationBone;
                m_savedOffset = original->m_vecOffset;
                m_savedRotation = original->m_vecRotation;
            }
        }

        Constraints* mount = player->getConstraintById(3);
        const unsigned desiredBone = NativeSamSheathBone(player);
        if (currentIsSam && current->m_pBehavior && mount &&
            mount->m_MainEntity.getEntity() == player->m_pEntity &&
            mount->m_ObjectEntityHandle.getEntity() == current &&
            mount->m_nBone == desiredBone && mount->m_nRotationBone == 0)
        {
            player->m_SheathHandle = current;
            current->m_pBehavior->onDisp();
            return true;
        }

        // Check if we already have an instantiated Sam sheath entity
        bool newlyCreated = false;
        Entity* sam = m_samSheath.getEntity();
        if (!sam || !sam->m_pBehavior)
        {
            // Verify pl1404 is loaded in memory
            if (!g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x11404), 0))
                return false;

            // Native Sam creation at RVA 0x46EC82 uses this entity name.
            sam = g_EntitySystem.createEntity("Sam_Sheath", static_cast<eObjID>(0x11404), nullptr);
            if (!sam || !sam->m_pBehavior)
                return false;

            m_samSheath = sam;
            newlyCreated = true;
        }

        // Hide original sheath (do not destroy it!)
        Entity* orig = m_originalSheath.getEntity();
        if (orig && orig != sam && orig->m_pBehavior)
        {
            orig->m_pBehavior->removeConstraint(4);
            orig->m_pBehavior->offDisp();
        }

        // Attach Sam's sheath to constraint 3
        player->removeConstraint(3);
        const unsigned samBone = NativeSamSheathBone(player);
        player->attachObject(3, player->m_pEntity, sam, samBone, 0);
        player->setConstraintsBone(3, samBone, 0);
        Constraints* c = player->getConstraintById(3);
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

        if (newlyCreated)
        {
            PrepareNativeSheath(sam);
        }
        sam->m_pBehavior->m_pOwner = player;
        sam->m_pBehavior->onDisp();

        player->m_pSheathEntity = sam;
        player->m_SheathHandle = sam;

        // Sync native engine constraint tracking fields so the engine's own behavior tick
        // (at 0x478E30) doesn't overwrite our constraint 17 to a stale bone.
        // [behavior+0x5444] = current desired bone, [behavior+0x5448] = last applied bone.
        *reinterpret_cast<unsigned*>(reinterpret_cast<char*>(player) + 0x5444) = samBone;
        *reinterpret_cast<unsigned*>(reinterpret_cast<char*>(player) + 0x5448) = samBone;

        g_GameFunctionManager.UpdateSwordConstraints(player);

        Trace("Sam_Sheath pl1404 attached; original retained for toggle-off");

        return true;
    }

    void RepositionSheathToHip(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            TrackPlayer(player);

            // Keep the original sheath and transform untouched until the Sam
            // archive and entity are ready. Update retries streaming failures.
            EnsureSamSheath(player);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            m_swapFaulted = true;
            Trace("swap fault: stopped retries for this activation; check installed pl1404 DAT/DTT texture pair");
        }
    }

    void RestoreSheathToBack(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            TrackPlayer(player);

            if (m_bAttachedToHand || player->getConstraintById(17))
            {
                player->removeConstraint(17);
                m_bAttachedToHand = false;
            }

            // Hide Sam's sheath if present
            Entity* sam = m_samSheath.getEntity();
            if (sam && sam->m_pBehavior)
            {
                sam->m_pBehavior->removeConstraint(4);
                sam->m_pBehavior->offDisp();
            }

            // Obtain Raiden's original sheath
            bool origNewlyCreated = false;
            Entity* orig = m_originalSheath.getEntity();
            if (!orig || !orig->m_pBehavior)
            {
                // In case player was spawned directly while Sam was active
                uint32_t origId = m_originalSheathId;
                if (origId && g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(origId), 0))
                {
                    orig = g_EntitySystem.createEntity("Pl0010_Sheath", static_cast<eObjID>(origId), nullptr);
                    if (orig && orig->m_pBehavior)
                    {
                        m_originalSheath = orig;
                        origNewlyCreated = true;
                    }
                }
            }

            if (orig && orig->m_pBehavior)
            {
                player->removeConstraint(3);
                const unsigned raidenBone = m_savedConstraint ? m_savedBone : NativeRaidenSheathBone();
                const unsigned raidenRotBone = m_savedConstraint ? m_savedRotationBone : 0;

                player->attachObject(3, player->m_pEntity, orig, raidenBone, raidenRotBone);
                player->setConstraintsBone(3, raidenBone, raidenRotBone);
                Constraints* c = player->getConstraintById(3);
                if (c)
                {
                    c->m_nBone = raidenBone;
                    c->m_nRotationBone = raidenRotBone;
                    c->m_vecOffset = m_savedConstraint ? m_savedOffset : Hw::cVec4(0, 0, 0, 0);
                    c->m_vecRotation = m_savedConstraint ? m_savedRotation : Hw::cVec4(0, 0, 0, 0);
                }

                if (origNewlyCreated)
                {
                    PrepareNativeSheath(orig);
                }
                orig->m_pBehavior->m_pOwner = player;
                orig->m_pBehavior->onDisp();

                player->m_pSheathEntity = orig;
                player->m_SheathHandle = orig;

                g_GameFunctionManager.UpdateSwordConstraints(player);
            }

            g_BxmFlagHandle.Reset_BxmFlagHandle_StaticVariable();
            m_bAttachedToHip = false;
            m_savedConstraint = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void SyncSheathAnimation(Pl0000* player)
    {
        if (!player || !m_bAttachedToHip || player->m_pSheathEntity != m_samSheath.getEntity()) return;
        __try {
            const bool direct=player->m_Rno0==SamUltimatePolicy::Action || player->m_Rno0==SamTemplateExecutionPolicy::Action;
            if(!direct){m_directCode[0]=0;m_chargePose=m_chargeGrip=false;}
            g_BxmFlagHandle.UpdateSheathAnimation(player,direct && m_directCode[0] ? m_directCode : nullptr,direct ? m_directSpeed : 1);
            // DLC sub_4645F0 copies slot 0's body frame to the sheath each tick.
            // SpeedTrack pauses/accelerates the body; an independent sheath
            // clock drifts even when both motions started in the same frame.
            Entity* sheath=m_samSheath.getEntity();
            if(sheath && sheath->m_pBehavior)
                reinterpret_cast<void(__thiscall*)(Behavior*,int,Behavior*)>(shared::base+0x695EE0)(
                    sheath->m_pBehavior,0,player);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
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

        TrackPlayer(player);
        if (toHip && !m_bAttachedToHip) m_swapFaulted = false;
        m_bAttachedToHip = toHip;

        if (toHip)
        {
            RepositionSheathToHip(player);
            SyncSheathAnimation(player);
        }
        else
        {
            m_chargePose=m_chargeGrip=false;m_directCode[0]=0;
            RestoreSheathToBack(player);
        }
    }

    void Update(Pl0000* player)
    {
        if (!player) return;
        __try
        {
            TrackPlayer(player);

            if (!m_bAttachedToHip || m_swapFaulted)
                return;

            // Check entity identity as well as the mount. A correct bone can
            // still belong to Raiden's sheath while pl1404 is streaming.
            if (!EnsureSamSheath(player)) return;

            Entity* sam = m_samSheath.getEntity();
            if (sam && player->m_pSheathEntity == sam)
            {
                const bool charge=m_chargePose && player->m_Rno0==SamUltimatePolicy::Action;
                g_BxmFlagHandle.CheckSheath_2097152Flag0(player,charge);
                g_BxmFlagHandle.CheckSheath_2097152Flag1(player,m_chargeGrip && player->m_Rno0==SamUltimatePolicy::Action);
                m_bAttachedToHand = player->getConstraintById(17) != nullptr;
                // Native visibility updates can re-enable the retained original.
                Entity* original = m_originalSheath.getEntity();
                if (original && original != sam && original->m_pBehavior)
                    original->m_pBehavior->offDisp();
            }

            SyncSheathAnimation(player);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            m_swapFaulted = true;
            Trace("update fault: stopped sheath retries until next activation");
        }
    }

    void AnimateSheathDirect(Pl0000* player, const char* code, float speed=1)
    {
        if (!player || !code || !m_bAttachedToHip || player->m_pSheathEntity != m_samSheath.getEntity()) return;
        SamSheathPolicy::Code(m_directCode,code);m_directSpeed=speed;
        __try { g_BxmFlagHandle.UpdateSheathAnimation(player, code, speed); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    bool IsAttachedToHip() const { return m_bAttachedToHip; }
    void SetChargePose(Pl0000* player,bool windup,bool release=false)
    { if(!player)return;TrackPlayer(player);m_chargePose=windup;m_chargeGrip=windup || release;Update(player); }
    bool Ready(Pl0000* player)const
    {return player && player==m_pLastPlayer && m_bAttachedToHip && !m_swapFaulted &&
        player->m_pSheathEntity==m_samSheath.getEntity() && player->m_pSheathEntity &&
        player->m_pSheathEntity->m_pBehavior && player->m_pSheathEntity->m_ObjId==static_cast<eObjID>(0x11404);}
};
