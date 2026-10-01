#pragma once

#include "1-GlobleVarious.h"
#include "1-GameFunctionManager.h"
#include "1-ChangePlayerManager.h"
#include "1-GameFileFlagCheck.h"
#include "1-PlayerActionHandler.h"
#include "1-PlayerStateManager.h"
#include "1-EffectManager.h"
#include "1-StyleSwitch.h"
#include "1-EventTriggerForSam.h"
#include "1-EnemyActionHandles.h"
#include "SamResourceManager.h"
#include "SamNativeRuntime.h"
#include "SheathController.h"
#include "ChargeController.h"
#include "SamBossSequence.h"
#include "SamUltimatePolicy.h"
#include "gui.h"
#include "SamDirectionalPolicy.h"
#include "SamRoundTripPolicy.h"
#include "SamRockProjectile.h"
#include "SamElectricCombat.h"
#include "SamStagePolicy.h"
#include <map>
#include <string>
#include "gui.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>

class SamMovesetManager
{
public:
    struct DebugSnapshot
    {
        bool requested = false;
        bool active = false;
        bool resourcesLoaded = false;
        bool lastReplacement = false;
        bool bossEnderActive = false;
        bool roundTripActive = false;
        unsigned int mapRequests = 0;
        unsigned int replacements = 0;
        unsigned int sequenceReplacements = 0;
        unsigned int failures = 0;
        unsigned int action0 = 0;
        unsigned int action1 = 0;
        unsigned int lastMapId = static_cast<unsigned int>(-1);
        unsigned int chargeFrames = 0;
        unsigned int chargeTier = 0;
        char handlerName[32] = "none";
        char raidenCode[16] = "-";
        char samCode[16] = "-";
        char archiveName[16] = "-";
        char animationName[96] = "-";
        char bossEnderCode[16] = "-";
        gui::HistoryEntry history[gui::MAX_HISTORY]{};
        size_t historyCount = 0;
        size_t pl1400FileCount = 0;
        size_t em0020FileCount = 0;
    };

private:
    // Requested is the user's G toggle. Active is only set after the DLC
    // archive has finished streaming and the routing has been installed.
    bool m_bEnabled = false;
    bool m_bActive = false;
    bool m_subweaponActive = false;
    ChargeController m_chargeController;
    SamElectricCombat m_electricCombat;
    bool m_finisherDown = false;
    Pl0000* m_activePlayer = nullptr;
    bool m_controllerInstalled = false;
    bool m_bBossEnderActive = false;
    unsigned int m_bossEnderTicks = 0;
    unsigned int m_bossEnderIndex = 0;
    const char* m_pBossEnderCode = nullptr;
    SamUltimatePolicy::Queue m_ultimateQueue;
    SamDirectionalPolicy::Input m_directionalInput;
    int m_pendingAddon = -1;
    unsigned m_addonTicks = 0;
    bool m_currentAddon = false;
    bool m_rtReleasePending = false;
    float m_stageFrame = 0.0f;
    unsigned m_stageLength = 1;
    bool m_slamDone = false, m_lightningHit = false;
    unsigned m_stormLifetime = 0, m_roundTripHits = 0;
    EntityHandle m_roundTripHandle;
    int m_savedSwordUpdate = 1;
    bool m_xDown = false;
    unsigned m_previousUltimatePad = 0;
    unsigned m_ultimateStage = 0;
    unsigned m_currentUltimate = 0;
    std::map<std::string, std::vector<uint8_t>> m_bossSequences;
    char m_lastComboClip[16]{};
    unsigned m_comboCount = 0;
    bool m_rockSpawned = false;
    cEspControler* m_pStormEsp = nullptr;
    cEspControler* m_pAddonChargeEsp = nullptr;
    bool m_addonChargeActive = false;
    bool m_stormActive = false;
    bool m_roundTripActive = false;
    unsigned int m_roundTripTicks = 0;
    unsigned int m_roundTripHitTicks = 0;
    Hw::cVec4 m_roundTripTarget{ 0.0f, 0.0f, 0.0f, 1.0f };
    bool m_drawThunderstormActive = false;
    unsigned int m_drawSlashStrikeTimer = 0;
    bool m_wasChargingDraw = false;
    DebugSnapshot m_debug{};

    static void CopyText(char* dst, size_t capacity, const char* src)
    {
        if (!dst || capacity == 0) return;
        std::snprintf(dst, capacity, "%s", src ? src : "-");
    }

    static void Log(const char* format, ...)
    {
        char line[512]{};
        va_list args;
        va_start(args, format);
        std::vsnprintf(line, sizeof(line), format, args);
        va_end(args);
        OutputDebugStringA(line);

        char exePath[MAX_PATH]{};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *lastSlash = '\0';
        char logPath[MAX_PATH]{};
        sprintf_s(logPath, "%s\\scripts\\RaidenMoveset-debug.log", exePath);

        FILE* file = nullptr;
        fopen_s(&file, logPath, "a");
        if (file)
        {
            std::fputs(line, file);
            std::fputc('\n', file);
            std::fclose(file);
        }
    }

    SamMovesetManager() = default;

public:
    static SamMovesetManager& Instance()
    {
        static SamMovesetManager s_instance;
        return s_instance;
    }

    bool IsEnabled() const { return m_bActive; }
    void SetSubweaponActive(bool active) { m_subweaponActive=active; }
    bool IsSubweaponActive() const { return m_subweaponActive; }
    bool IsRequested() const { return m_bEnabled; }

    void EndRoundTrip(Pl0000* player)
    {
        if (!m_roundTripActive) return;
        m_roundTripActive = false;
        m_rtReleasePending = false;
        m_roundTripHandle = nullptr;
        Log("[SamMoveset] ROUND TRIP catch after %u ticks / %u hits",m_roundTripTicks,m_roundTripHits);
        g_GameStateManager.IsRoundTripActive = false;
        m_roundTripTicks = 0;
        m_roundTripHitTicks = 0;
        if (player)
        {
            try
            {
                reinterpret_cast<void(__thiscall*)(Pl0000*, BOOL)>(shared::base + 0x77E210)(player, FALSE);
                player->m_bSwordHidden = 0;
                player->field_13F4 = 1;
                player->field_13F8 = m_savedSwordUpdate;

                Entity* sword = player->m_pBladeEntity ? player->m_pBladeEntity : player->m_SwordHandle.getEntity();
                if (sword)
                {
                    player->removeConstraint(4);
                    player->attachObject(4, player->m_pEntity, sword, 1792, 0);
                    player->setConstraintsBone(4, 1792, 0);
                    if (sword->m_pBehavior)
                    {
                        sword->m_pBehavior->onDisp();
                        Pl0012* swordInstance = reinterpret_cast<Pl0012*>(sword->m_pBehavior);
                        ((char* (__thiscall*)(int))(shared::base + 0x811BE0))(reinterpret_cast<int>(swordInstance));
                    }
                }

                g_GameFunctionManager.UpdateSwordConstraints(player);

                if (player->m_Rno0 == 0x100008 || player->m_Rno0 == 0x10000E || player->isUnarmed())
                {
                    const bool air = player->isInAir() != FALSE;
                    player->setRno(SamTogglePolicy::EntryState(air), 0, 0, 0);
                    if (!air) player->requestAnimationByMap(4);
                }

                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_ripper_in", 0);
                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_sys_item_electro_repair", 0);
                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
            }
            catch (...) {}
        }
        Log("[SamMoveset] ROUND TRIP ended: sword caught, armed combat restored");
    }

    static int AttackPower(Pl0000* player, int number)
    {
        if (!player || !player->m_pBattleParameterImplement) return 0;
        // A missing or corrupt entry must never become negative/healing damage.
        int power = reinterpret_cast<int(__thiscall*)(Pl0000*, int)>(shared::base + 0x77ED30)(player, number);
        return power > 0 && power <= 10000 ? power : 0;
    }

    void UpdateRoundTrip(Pl0000* player)
    {
        if (!m_roundTripActive || !player) return;
        if (!player->isAlive())
        {
            EndRoundTrip(player);
            return;
        }
        Entity* blade = player->m_pBladeEntity ? player->m_pBladeEntity : player->m_SwordHandle.getEntity();
        if (!blade)
        {
            EndRoundTrip(player);
            return;
        }

        player->removeConstraint(4);
        if (player->m_pSheathEntity && player->m_pSheathEntity->m_pBehavior)
            player->m_pSheathEntity->m_pBehavior->removeConstraint(4);
        player->field_13F4 = 0;
        player->field_13F8 = 0; // suppress native hand/sheath reattachment
        player->m_bSwordHidden = 0;
        // Ensure blade visibility while in flight
        if (blade->m_pBehavior)
            blade->m_pBehavior->onDisp();

        ++m_roundTripTicks;

        // Early recall check: player pressed heavy attack, X key, or taunt while blade is out (after 25 ticks)
        if (m_roundTripTicks > 25 && m_roundTripTicks < SamRoundTripPolicy::ReturnTick)
        {
            const bool heavyPressed = (player->m_ButtonHeavyAttack & player->m_CurrentInput.m_Trig) != 0;
            const bool xPressed = (GetAsyncKeyState('X') & 0x8000) != 0;
            const bool tauntPressed = ((g_dbPad.m_Trig & 0x0001) != 0) || ((GetAsyncKeyState('T') & 0x8000) != 0);
            if (heavyPressed || xPressed || tauntPressed)
            {
                m_roundTripTicks = SamRoundTripPolicy::ReturnTick;
                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_ripper_in", 0);
                Log("[SamMoveset] ROUND TRIP early recall initiated by player input");
            }
        }

        Entity* target = m_roundTripHandle.getEntity();
        if (!SamTargets::Enemy(target))
        { target = SamTargets::Find(player); m_roundTripHandle = target; }
        if (target && target->isAlive())
        {
            m_roundTripTarget = target->getTransPos();
            m_roundTripTarget.y += 1.0f;
        }

        Hw::cVec4 bladePos = blade->getTransPos();

        // Phase 1 (0 to 130 ticks, ~2.2 seconds): Homing to target & rapid spin attack
        if (m_roundTripTicks < SamRoundTripPolicy::ReturnTick)
        {
            float dx = m_roundTripTarget.x - bladePos.x;
            float dy = m_roundTripTarget.y - bladePos.y;
            float dz = m_roundTripTarget.z - bladePos.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist > 0.35f)
            {
                SamRoundTripPolicy::Step(bladePos.x, bladePos.y, bladePos.z,
                    m_roundTripTarget.x, m_roundTripTarget.y, m_roundTripTarget.z, 0.55f);
                blade->m_pBehavior->setTransPos(bladePos);
                blade->setTransPos(bladePos);
            }
            blade->m_pBehavior->setRotation(blade->getRot() + Hw::cVec4(0.20f, 0.65f, 0.0f, 0.0f));
            blade->addRot(Hw::cVec4(0.20f, 0.65f, 0.0f, 0.0f));
            ((void(__thiscall*)(Entity*, float))(shared::base + 0x1CC40))(blade, 35.0f);

            if (++m_roundTripHitTicks >= 16 && m_roundTripHits < 8)
            {
                m_roundTripHitTicks = 0;
                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
                ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("pl1400_se_swd_scrape", 0);

                if (target && target->isAlive() && target->m_pBehavior &&
                    SamRoundTripPolicy::CanHit(m_roundTripTarget.x - bladePos.x,
                        m_roundTripTarget.y - bladePos.y, m_roundTripTarget.z - bladePos.z, true))
                {
                    try
                    {
                        auto* enemy = static_cast<BehaviorAppBase*>(target->m_pBehavior);
                        enemy->damage(SamBalancePolicy::ProjectileDamage(AttackPower(player,4),enemy->m_HpMax),false);
                        ++m_roundTripHits;
                    }
                    catch (...) {}
                }
            }
        }
        // Phase 2 (130 to 260 ticks): Return homing to player
        else
        {
            Hw::cVec4 returnPos = player->m_TransPos;
            returnPos.y += 1.0f;
            float dx = returnPos.x - bladePos.x;
            float dy = returnPos.y - bladePos.y;
            float dz = returnPos.z - bladePos.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist > 0.75f && m_roundTripTicks < SamRoundTripPolicy::TimeoutTick)
            {
                SamRoundTripPolicy::Step(bladePos.x, bladePos.y, bladePos.z,
                    returnPos.x, returnPos.y, returnPos.z, 0.70f);
                blade->m_pBehavior->setTransPos(bladePos);
                blade->setTransPos(bladePos);
                blade->addRot(Hw::cVec4(0.0f, 0.85f, 0.0f, 0.0f));
                ((void(__thiscall*)(Entity*, float))(shared::base + 0x1CC40))(blade, 35.0f);

                if (++m_roundTripHitTicks >= 16 && m_roundTripHits < 8)
                {
                    m_roundTripHitTicks = 0;
                    if (target && target->isAlive() && target->m_pBehavior)
                    {
                        Hw::cVec4 tPos = target->getTransPos();
                        float tdx = tPos.x - bladePos.x;
                        float tdy = tPos.y - bladePos.y;
                        float tdz = tPos.z - bladePos.z;
                        if (SamRoundTripPolicy::CanHit(tdx, tdy, tdz, true))
                        {
                            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
                            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("pl1400_se_swd_scrape", 0);
                            try
                            {
                                auto* enemy = static_cast<BehaviorAppBase*>(target->m_pBehavior);
                                enemy->damage(SamBalancePolicy::ProjectileDamage(AttackPower(player,4),enemy->m_HpMax),false);
                                ++m_roundTripHits;
                            }
                            catch (...) {}
                        }
                    }
                }
            }
            else
            {
                EndRoundTrip(player);
            }
        }
    }

    void ForgetPlayer(Pl0000* player)
    {
        if (m_activePlayer != player) return;
        SheathController::Instance().SetSheathToHip(player, false);
        if (m_roundTripActive)
            EndRoundTrip(player);
        m_electricCombat.Reset();
        m_bActive = m_bEnabled = m_controllerInstalled = false;
        m_subweaponActive = false;
        m_activePlayer = nullptr;
        ResetUltimate();
        g_GameStateManager.IsMainSamPlayer = false;
        g_GameStateManager.IsStyleChanged = false;
        g_ChangePlayerHandle.ChangeModelID();
    }

    void CopyDebugSnapshot(DebugSnapshot& out) const
    {
        out = m_debug;
        out.requested = m_bEnabled;
        out.active = m_bActive;
        out.resourcesLoaded = SamResourceManager::Instance().IsLoaded();
        out.pl1400FileCount = SamResourceManager::Instance().GetPl1400FileCount();
        out.em0020FileCount = SamResourceManager::Instance().GetEm0020FileCount();
        out.chargeFrames = m_chargeController.GetFrames();
        out.chargeTier = m_chargeController.GetTier();
        out.bossEnderActive = m_bBossEnderActive;
        out.roundTripActive = m_roundTripActive;
        CopyText(out.bossEnderCode, sizeof(out.bossEnderCode), m_pBossEnderCode);
        if (m_activePlayer)
        {
            out.action0 = m_activePlayer->m_Rno0;
            out.action1 = m_activePlayer->m_Rno1;
            if (m_activePlayer->m_pAnimationSlot && m_activePlayer->m_pAnimationSlot->m_pArray)
                CopyText(out.animationName, sizeof(out.animationName),
                    m_activePlayer->m_pAnimationSlot->m_pArray[0].m_pAnimName);
        }
    }

    void TraceRequest(const char* name)
    {
        static unsigned count = 0;
        if (count++ < 120)
            Log("[SamMoveset] LIVE request name=%s action=%X/%u", name ? name : "null",
                m_activePlayer ? m_activePlayer->m_Rno0 : 0,
                m_activePlayer ? m_activePlayer->m_Rno1 : 0);
    }

    void RecordActionHandler(const char* handler)
    {
        CopyText(m_debug.handlerName, sizeof(m_debug.handlerName), handler);
    }

    void RecordAnimationMapRequest(int animationId, const char* raidenCode,
        const char* samCode, const char* archive, bool replaced, const char* details = nullptr)
    {
        ++m_debug.mapRequests;
        if (replaced) ++m_debug.replacements; else ++m_debug.failures;
        m_debug.lastMapId = static_cast<unsigned int>(animationId);
        m_debug.lastReplacement = replaced;
        CopyText(m_debug.raidenCode, sizeof(m_debug.raidenCode), raidenCode);
        CopyText(m_debug.samCode, sizeof(m_debug.samCode), samCode);
        CopyText(m_debug.archiveName, sizeof(m_debug.archiveName), archive);

        static unsigned int s_frameCounter = 0;
        ++s_frameCounter;

        if (m_debug.historyCount < gui::MAX_HISTORY)
            ++m_debug.historyCount;

        for (size_t i = m_debug.historyCount - 1; i > 0; --i)
        {
            m_debug.history[i] = m_debug.history[i - 1];
        }

        auto& entry = m_debug.history[0];
        entry.frame = s_frameCounter;
        entry.animId = animationId;
        CopyText(entry.raidenCode, sizeof(entry.raidenCode), raidenCode);
        CopyText(entry.samCode, sizeof(entry.samCode), samCode);
        CopyText(entry.archiveName, sizeof(entry.archiveName), archive);
        entry.replaced = replaced;
        CopyText(entry.details, sizeof(entry.details), details ? details : (replaced ? "Replaced with Sam motion" : "Fallback to Raiden native"));

        Log("[SamMoveset] #%u map=%d raiden=%s sam=%s archive=%s -> %s (%s)",
            s_frameCounter, animationId, raidenCode ? raidenCode : "-",
            samCode ? samCode : "-", archive ? archive : "-",
            replaced ? "SUCCESS" : "FALLBACK", entry.details);
    }

    void RecordSequenceReplacement(const char* code)
    {
        ++m_debug.sequenceReplacements;
        Log("[SamMoveset] SEQUENCE supplied: code=%s total=%u", code, m_debug.sequenceReplacements);
    }

    static const char* ResolveSamCode(const char* inCode)
    {
        if (!inCode || !inCode[0]) return nullptr;

        const char* p = inCode;
        const char* underscore = std::strchr(p, '_');
        if (underscore && std::strlen(underscore + 1) >= 4)
        {
            p = underscore + 1;
        }

        // 1. Reverse map: if inCode matches pair.to (e.g. 9000, 9100), return pair.from (2000, 2020)
        for (const auto& pair : kSamAnimationPairs)
        {
            if (std::strncmp(p, pair.to, 4) == 0)
                return pair.from;
        }

        // 2. Mod 9xxx convention mappings: 9xxx -> 2xxx
        static thread_local char s_mapped[8];
        if (p[0] == '9' && std::strlen(p) >= 4)
        {
            std::snprintf(s_mapped, sizeof(s_mapped), "2%.3s", p + 1);
            return s_mapped;
        }

        // 3. If it's a 4-char valid code (e.g. 2000, 2020, 2510), return it directly
        if (SamArchiveLookup::ValidCode(p))
        {
            static thread_local char s_code[8];
            std::memcpy(s_code, p, 4);
            s_code[4] = '\0';
            return s_code;
        }

        return inCode;
    }

    static bool ResolveSamClip(const char* code, bool requireSequence)
    {
        const char* samCode = ResolveSamCode(code);
        const auto clip = SamResourceManager::Instance().GetClip(samCode ? samCode : code,
            SamArchiveLookup::Source::Playable, true, requireSequence);
        return clip.motion != nullptr && (!requireSequence || clip.sequence != nullptr);
    }

    void ResetForNativeDlc(Pl0000* player = nullptr)
    {
        m_bEnabled = false;
        Deactivate(player);
        g_GameStateManager.IsMainSamPlayer = false;
        g_GameStateManager.InstantCharge = false;
    }

private:
    void Activate(Pl0000* player)
    {
        if (!player || m_bActive || !SamResourceManager::Instance().IsLoaded())
            return;

        if (!SamNativeRuntime::Get().Activate(player)) return;
        // Sam DLC charge/finisher nodes require the sword, not a secondary
        // weapon or Raiden's sword-lost punch mode.
        if(g_pPlayerManager) g_pPlayerManager->setCustomWeaponEquipped(0);
        reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base+0x77D0E0)(player);
        player->setSwordLost(FALSE);
        m_bActive = true;
        m_activePlayer = player;
        g_GameStateManager.IsMainSamPlayer = true;
        g_GameStateManager.IsStyleChanged = true;

        m_controllerInstalled = true;

        ResetUltimate();
        m_directionalInput.Reset();

        // Switch model IDs to Sam's Murasama sheath/sword and re-attach sheath to hip
        g_ChangePlayerHandle.ChangeModelID();
        SheathController::Instance().SetSheathToHip(player, true);

        Log("[SamMoveset] ACTIVATED smooth-switch-8: Sam update/context/attack table; light=%d heavy=%d charge=%d speed=1.20 player=%p",
            player->m_pBattleParameterImplement->getAttackPowerByNo(4),
            player->m_pBattleParameterImplement->getAttackPowerByNo(12),
            player->m_pBattleParameterImplement->getAttackPowerByNo(26),player);

        // Sound cues for activation
        ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_sys_decide_l", 0);
        ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
    }

    void Deactivate(Pl0000* player)
    {
        if (!m_bActive && !m_bEnabled)
            return;

        if (m_roundTripActive)
            EndRoundTrip(player ? player : m_activePlayer);
        m_electricCombat.Reset();
        m_stormLifetime=0; StopThunderstorm();
        m_drawThunderstormActive = false;
        m_drawSlashStrikeTimer = 0;
        m_wasChargingDraw = false;

        // Restore sheath to back and revert model IDs
        SheathController::Instance().SetSheathToHip(player ? player : m_activePlayer, false);

        m_bActive = false;
        m_subweaponActive = false;
        m_bEnabled = false;
        g_GameStateManager.IsMainSamPlayer = false;
        g_GameStateManager.IsStyleChanged = false;
        g_GameStateManager.InstantCharge = false;
        g_ChangePlayerHandle.ChangeModelID();
        m_chargeController.Reset();
        m_bBossEnderActive = false;
        m_bossEnderTicks = 0;
        m_pBossEnderCode = nullptr;

        ResetUltimate();

        // Restore original Raiden vtable & animation map
        if (m_activePlayer && SamNativeRuntime::Get().Owns(m_activePlayer))
            SamNativeRuntime::Get().Deactivate(m_activePlayer);
        else if (player && SamNativeRuntime::Get().Owns(player))
            SamNativeRuntime::Get().Deactivate(player);

        m_controllerInstalled = false;
        m_activePlayer = nullptr;

        Log("[SamMoveset] DEACTIVATED successfully");
        ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_sys_cancel_l", 0);
    }

public:
    void StartThunderstorm(Pl0000* player)
    {
        if (!player || m_stormActive) return;
        try
        {
            if (!m_pStormEsp)
                m_pStormEsp = new cEspControler();
            SamElectricCombat::Lightning(player,player->m_pEntity,m_pStormEsp);
            m_electricCombat.Strike(player);
            m_stormActive = true;
            m_stormLifetime = 75;
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_ripper_in", 0);
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_sys_item_electro_repair", 0);
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
            if (player->m_pBladeEntity)
                ((void(__thiscall*)(Entity*, float))(shared::base + 0x1CC40))(player->m_pBladeEntity, 30.0f);
            Log("[SamMoveset] ULTIMATE thunderstorm active: rain and lightning engaged");
        }
        catch (...) {}
    }

    void StopThunderstorm()
    {
        if (!m_stormActive || !m_pStormEsp) return;
        try
        {
            m_pStormEsp->FadeUnits(4.0f, 0.0f);
            m_stormActive = false;
            Log("[SamMoveset] ULTIMATE thunderstorm fading out");
        }
        catch (...) {}
    }

    static bool IsThunderMove(unsigned index)
    {
        if (index >= SamUltimatePolicy::Count) return false;
        const char* name = SamUltimatePolicy::Moves[index].name;
        return (std::strstr(name, "thunder") != nullptr || std::strstr(name, "lightning") != nullptr);
    }

    void CycleUltimate()
    {
        ++m_bossEnderIndex;
    }

    const char* NextUltimateName() const
    {
        return SamUltimatePolicy::Moves[SamUltimatePolicy::UltimateIndex(m_bossEnderIndex)].name;
    }
    bool UltimateQueued() const { return m_ultimateQueue.pending; }
    bool IsRoundTripActive() const { return m_roundTripActive; }
    void SetUltimateIndex(unsigned int index)
    {
        m_bossEnderIndex = index % SamUltimatePolicy::UltimateCount;
    }
    void TriggerUltimateNow()
    {
        if (m_activePlayer)
        {
            m_ultimateQueue.Request();
            Log("[SamMoveset] ULTIMATE triggered via GUI: %s", NextUltimateName());
        }
    }
    void ResetUltimate()
    {
        StopAddonCharge();
        m_ultimateQueue.Reset();
        m_pendingAddon = -1;
        m_addonTicks = 0;
        m_rtReleasePending = false;
        m_bBossEnderActive = false;
        m_bossEnderTicks = 0;
        m_electricCombat.EndGrab();
        m_ultimateStage = 0;
        m_pBossEnderCode = nullptr;
        m_comboCount = 0;
        m_lastComboClip[0] = 0;
        m_xDown = (GetAsyncKeyState('X') & 0x8000) != 0;
        m_previousUltimatePad=g_dbPad.m_On|g_dbPad.m_Trig;
        if (!m_drawThunderstormActive && m_drawSlashStrikeTimer == 0 && !m_stormLifetime)
            StopThunderstorm();
        if (m_activePlayer) SamNativeRuntime::Get().ConfigureDamage(m_activePlayer,false);
        m_lightningHit = m_slamDone = false;
    }

    void StopAddonCharge()
    {
        if (m_addonChargeActive && m_pAddonChargeEsp) m_pAddonChargeEsp->FadeUnits(2.0f,0.0f);
        m_addonChargeActive=false;
    }

    void* BossSequence(const char* code, void* sequence)
    {
        auto it = m_bossSequences.find(code);
        if (it != m_bossSequences.end()) return it->second.data();
        auto copy = SamBossSequence::Adapt(sequence, SamResourceManager::Instance().GetBossSequenceSize(sequence), code);
        if (copy.empty()) return nullptr;
        auto inserted = m_bossSequences.emplace(code, std::move(copy));
        return inserted.first->second.data();
    }

    bool PlayBossStage(Pl0000* player, const char* code)
    {
        const auto clip = SamResourceManager::Instance().GetClip(code,
            SamArchiveLookup::Source::Boss, false, true);
        if (!clip.motion || !clip.sequence) return false;
        void* sequence = BossSequence(code, clip.sequence);
        if (!sequence) return false;
        m_stageLength = SamStagePolicy::Length(clip.motion);
        if (!m_stageLength || m_stageLength > 3600) return false;
        m_stageFrame = 0.0f;
        m_slamDone = m_lightningHit = false;
        SamNativeRuntime::Get().ConfigureDamage(player,true,SamBossSequence::HitCount(sequence));
        const int animation = player->setDirectAnimation(clip.motion, sequence, 0,
            0.05f, 1.0f, 0x8000000, 0.0f, SamBalancePolicy::AttackSpeed);
        if (animation == -1) return false;
        StopAddonCharge();
        if (!std::strcmp(code,"3000") || !std::strcmp(code,"3500"))
        {
            if (!m_pAddonChargeEsp) m_pAddonChargeEsp=new cEspControler();
            SamNativeRuntime::Get().CreatePlayerEffect(player,117,m_pAddonChargeEsp);
            m_addonChargeActive=true;
        }
        SheathController::Instance().AnimateSheathDirect(player, code);
        if ((!std::strcmp(code, "a648") || !std::strcmp(code, "a646")) && m_ultimateStage == 0)
            m_electricCombat.Grab(player);
        m_pBossEnderCode = code;
        m_bossEnderTicks = 0;
        m_rockSpawned = false;
        if (!std::strcmp(code, "92e4"))
        {
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
            if (player->m_pBladeEntity)
                ((void(__thiscall*)(Entity*, float))(shared::base + 0x1CC40))(player->m_pBladeEntity, 25.0f);
        }
        else if (!std::strcmp(code, "3210"))
            m_rtReleasePending = true;
        else if (!std::strcmp(code, "a648"))
        {
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("em0020_se_mov_grab", 0);
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("em0020_se_atk_tackle", 0);
        }
        else if (!std::strcmp(code, "3017"))
        {
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
            ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_sys_item_electro_repair", 0);
            if (player->m_pBladeEntity)
                ((void(__thiscall*)(Entity*, float))(shared::base + 0x1CC40))(player->m_pBladeEntity, 45.0f);
        }
        RecordAnimationMapRequest(-1, code, code, "em0020", true, m_currentAddon ? "directional add-on: player-adapted boss sequence" : "X ultimate: player-adapted boss sequence");
        RecordSequenceReplacement(code);
        Log("[SamMoveset] ULTIMATE stage=%s frames=%u boxes=%u name=%s", code,
            m_stageLength,SamBossSequence::HitCount(sequence),SamUltimatePolicy::Moves[m_currentUltimate].name);
        return true;
    }

    void LaunchRoundTrip(Pl0000* player)
    {
        m_rtReleasePending = false;
        Entity* blade = player->m_pBladeEntity ? player->m_pBladeEntity : player->m_SwordHandle.getEntity();
        if (!blade || !blade->m_pBehavior) return;
        m_roundTripActive = true;
        g_GameStateManager.IsRoundTripActive = true;
        m_roundTripTicks = 0;
        m_roundTripHitTicks = 0;
        m_roundTripHits = 0;
        m_savedSwordUpdate = player->field_13F8;
        player->field_13F8 = 0;
        m_roundTripHandle = SamTargets::Find(player);

        Entity* sheath = player->getConstraintsEntity(3);
        if (sheath && sheath->m_pBehavior)
            sheath->m_pBehavior->removeConstraint(4);
        player->removeConstraint(4);

        reinterpret_cast<void(__thiscall*)(Pl0000*, BOOL)>(shared::base + 0x77E210)(player, TRUE);
        player->m_bSwordHidden = 0; // physical blade remains visible in flight
        player->field_13F4 = 0;

        const float yaw = player->m_Rot.y;
        if (blade)
        {
            if (blade->m_pBehavior)
                blade->m_pBehavior->onDisp();

            Hw::cVec4 startPos = player->m_TransPos;
            startPos.x += std::sin(yaw) * 1.5f;
            startPos.z += std::cos(yaw) * 1.5f;
            startPos.y += 1.2f;
            blade->m_pBehavior->setTransPos(startPos);
            blade->setTransPos(startPos);
        }

        Entity* target = SamTargets::Find(player);
        if (target && target->isAlive())
        {
            m_roundTripTarget = target->getTransPos();
            m_roundTripTarget.y += 1.0f;
        }
        else
        {
            m_roundTripTarget = player->m_TransPos;
            m_roundTripTarget.x += std::sin(yaw) * 7.5f;
            m_roundTripTarget.z += std::cos(yaw) * 7.5f;
            m_roundTripTarget.y += 1.0f;
        }
        ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("em0020_se_atk_counter_throw", 0);
        ((int(__cdecl*)(const char*, int))(shared::base + 0xA5E050))("core_se_btl_char_zan", 0);
        Log("[SamMoveset] ROUND TRIP sword thrown! Raiden switched to unarmed mode");
    }

    void FinishUltimate(Pl0000* player)
    {
        const bool owns = player->m_Rno0 == SamUltimatePolicy::Action;
        ResetUltimate();
        if (owns)
        {
            if (m_roundTripActive)
            {
                player->setRno(0x100000, 0, 0, 0);
                player->requestAnimationByName("0000", 0, 0.05f, 1.0f, 0, 0.0f, 1.0f);
            }
            else
            {
                const bool air = player->isInAir() != FALSE;
                player->setRno(SamTogglePolicy::EntryState(air), 0, 0, 0);
                if (air) player->requestAnimationByName("0232", 0, 0.05f, 1.0f, 0, 0.0f, 1.0f);
                else player->requestAnimationByMap(4);
            }
        }
        Log("[SamMoveset] ULTIMATE finished/cancelled");
    }

    void UpdateUltimate(Pl0000* player, bool focused)
    {
        const bool down = (GetAsyncKeyState('X') & 0x8000) != 0;
        const bool pressed = focused && SamUltimatePolicy::ReserveButton(player->m_Rno0,
            player->isAlive()!=FALSE,player->isInAir()!=FALSE,
            Trigger::StpFlags.STP_OBJ||g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT,
            player->isBladeModeActive()!=FALSE,m_subweaponActive) && ((down && !m_xDown) ||
            SamUltimatePolicy::Press(g_dbPad.m_On,g_dbPad.m_Trig,m_previousUltimatePad));
        m_previousUltimatePad=g_dbPad.m_On|g_dbPad.m_Trig;
        m_xDown = down;
        const bool finisherDown = (GetAsyncKeyState('F') & 0x8000) != 0;
        const bool finisherPressed = focused && finisherDown && !m_finisherDown;
        m_finisherDown = finisherDown;
        const bool airborne = player->isInAir() != FALSE;
        const bool alive = player->isAlive() != FALSE;
        const unsigned action = player->m_Rno0;
        RecordActionHandler("sam-native");
        const bool charging = ChargeController::IsChargeAction(action) || action == 0x100015 || action == 0x100016;
        const bool heavyHeld = ((player->m_ButtonHeavyAttack & player->m_CurrentInput.m_On) != 0);
        m_chargeController.Tick(charging && heavyHeld, airborne);

        // Paired Sam sequences and Sam's native charge handler own charge FX.
        // Keep diagnostics separate from effect playback.
        float stickX = g_dbPad.m_LeftStick.x, stickY = g_dbPad.m_LeftStick.y;
        if (focused)
        {
            if (GetAsyncKeyState('A') & 0x8000) stickX -= 1.0f;
            if (GetAsyncKeyState('D') & 0x8000) stickX += 1.0f;
            if (GetAsyncKeyState('W') & 0x8000) stickY += 1.0f;
            if (GetAsyncKeyState('S') & 0x8000) stickY -= 1.0f;
        }
        const bool lightTrig = (player->m_ButtonLightAttack & player->m_CurrentInput.m_Trig) != 0;
        const bool heavyTrig = (player->m_ButtonHeavyAttack & player->m_CurrentInput.m_Trig) != 0;
        const int addon = m_directionalInput.Poll(stickX, stickY, focused && lightTrig, focused && heavyTrig);
        if (focused && alive && !airborne && !m_roundTripActive && !m_bBossEnderActive &&
            (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action)) && addon >= 0)
        {
            m_pendingAddon = addon;
            m_addonTicks = 18;
            m_ultimateQueue.Reset();
        }
        if (finisherPressed && alive && !airborne && !m_roundTripActive && !m_bBossEnderActive &&
            (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action)) &&
            m_electricCombat.FinisherAvailable(player))
        {
            m_pendingAddon = 3;
            m_addonTicks = 18;
            m_ultimateQueue.Reset();
        }
        if (m_addonTicks && --m_addonTicks == 0) m_pendingAddon = -1;
        if (!focused || !alive || airborne ||
            (!SamUltimatePolicy::Neutral(action) && !SamUltimatePolicy::Attack(action) && action != SamUltimatePolicy::Action))
            m_pendingAddon = -1;
        if (m_bBossEnderActive)
        {
            if (action != SamUltimatePolicy::Action)
            {
                ResetUltimate(); // damage/cutscene/other engine transition wins
                return;
            }
            if (airborne || !alive || ++m_bossEnderTicks > 600)
            {
                FinishUltimate(player);
                return;
            }
            m_stageFrame += SamBalancePolicy::AttackSpeed * (std::max)(0.0f,player->getSpeedRate());
            const int animationFrame = int(m_stageFrame);
            if (m_rtReleasePending && m_ultimateStage == 1 && animationFrame >= SamRoundTripPolicy::ReleaseFrame)
                LaunchRoundTrip(player);
            if (m_currentUltimate == 0 && m_ultimateStage == 1 && !m_rockSpawned && animationFrame > 22)
            {
                m_rockSpawned = true;
                const bool spawned = SamRockProjectile::Spawn(player);
                Log("[SamMoveset] ULTIMATE stone projectile=%s", spawned ? "spawned" : "unavailable");
            }
            if ((m_currentUltimate == 10 || m_currentUltimate == 11) && m_ultimateStage == 1 &&
                !m_slamDone && animationFrame >= 20)
            { m_slamDone = true; Log("[SamMoveset] Grab impact target=%s",m_electricCombat.Slam(player)?"hit":"miss"); }
            if (IsThunderMove(m_currentUltimate) && m_ultimateStage == 1 && !m_lightningHit && animationFrame >= 20)
            { m_lightningHit=true; m_electricCombat.Strike(player,true); }
            if (IsThunderMove(m_currentUltimate) && m_ultimateStage == 1 && !m_stormActive)
            {
                StartThunderstorm(player);
            }
            // Direct clips are not represented by Raiden's old map-slot end
            // flag. Their own MOT duration governs transition/release timing.
            if (SamStagePolicy::Finished(m_stageFrame,m_stageLength))
            {
                if (m_ultimateStage == 0)
                {
                    m_ultimateStage = 1;
                    if (!PlayBossStage(player, SamUltimatePolicy::Moves[m_currentUltimate].release))
                        FinishUltimate(player);
                }
                else FinishUltimate(player);
            }
            return;
        }

        m_ultimateQueue.Tick(action, alive, airborne);
        if (!alive || airborne) { m_comboCount = 0; return; }
        if (!m_roundTripActive && pressed && (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action)))
        {
            m_ultimateQueue.Request();
            Log("[SamMoveset] ULTIMATE queued by X/controller B: %s", NextUltimateName());
        }
        if (m_roundTripActive || (!m_ultimateQueue.pending && m_pendingAddon < 0)) return;
        const bool finished = g_GameFunctionManager.IsAnimationEnded(player, 0);
        const bool addonCancel = m_pendingAddon >= 0 && SamUltimatePolicy::Attack(action) &&
            !charging && (player->ckSeqFlag(1) || player->ckSeqFlag(2) || player->ckSeqFlag(24));
        if (!addonCancel && !SamUltimatePolicy::CanStart(action, alive, airborne, finished)) return;
        m_currentAddon = m_pendingAddon >= 0;
        m_currentUltimate = m_currentAddon ? static_cast<unsigned>(m_pendingAddon) :
            SamUltimatePolicy::UltimateIndex(m_bossEnderIndex);
        const auto& move = SamUltimatePolicy::Moves[m_currentUltimate];
        // Preflight both stages before taking ownership of combat dispatch.
        for (const char* code : {move.windup, move.release})
        {
            if (!code) continue;
            const auto clip = SamResourceManager::Instance().GetClip(code, SamArchiveLookup::Source::Boss, false, true);
            if (!clip.motion || !clip.sequence || !BossSequence(code, clip.sequence))
            {
                m_ultimateQueue.Reset();
                m_pendingAddon = -1;
                Log("[SamMoveset] ULTIMATE unavailable: missing boss pair %s", code);
                return;
            }
        }
        m_ultimateQueue.Reset();
        m_pendingAddon = -1;
        m_bBossEnderActive = true;
        m_ultimateStage = move.windup ? 0 : 1;
        player->m_CurrentInput.m_Trig &= ~(player->m_ButtonLightAttack | player->m_ButtonHeavyAttack);
        player->setRno(SamUltimatePolicy::Action, 0, 0, 0);
        if (!PlayBossStage(player, move.windup ? move.windup : move.release)) FinishUltimate(player);
        else if (!m_currentAddon) ++m_bossEnderIndex;
    }

    void Toggle(Pl0000* player)
    {
        if (!player) return;

        if (m_bActive || m_bEnabled)
        {
            Deactivate(player);
        }
        else
        {
            m_bEnabled = true;
            Log("[SamMoveset] G toggle ON requested");
            SamResourceManager::Instance().RequestAllSamResources();
            SamResourceManager::Instance().Update();
            Activate(player);
        }
    }

    void SetEnabled(Pl0000* player,bool enabled)
    {
        if(!player) return;
        if(enabled)
        {
            if(!m_bEnabled && !m_bActive) Toggle(player);
        }
        else if(m_bEnabled || m_bActive) Deactivate(player);
    }

    void OnTick(Pl0000* player)
    {
        if (!player || reinterpret_cast<uintptr_t>(player) < 0x10000) return;
        if (!player->m_pEntity || reinterpret_cast<uintptr_t>(player->m_pEntity) < 0x10000) return;

        if (m_activePlayer && m_activePlayer != player)
            Deactivate(player);

        // Service DLC resource streaming every tick
        SamResourceManager::Instance().Update();

        // Toggle check: 'G' on keyboard, or Back/Select on controller
        static bool s_wasGDown = false;
        bool isGDown = (GetAsyncKeyState('G') & 0x8000) != 0;
        bool kbToggle = isGDown && !s_wasGDown;
        s_wasGDown = isGDown;

        // Controller toggle: Back / Select (0x0020), or L3 + R3
        static_assert(SamUltimatePolicy::ControllerButton==Hw::PAD_BTN_B && SamUltimatePolicy::SelectButton==Hw::PAD_BTN_SL);
        const bool padDown = ((g_dbPad.m_Trig & SamUltimatePolicy::SelectButton) != 0) ||
            ((g_dbPad.m_On & SamUltimatePolicy::LeftStickButton) && (g_dbPad.m_Trig & SamUltimatePolicy::RightStickButton));
        static bool s_wasPadDown = false;
        bool padToggle = padDown && !s_wasPadDown;
        s_wasPadDown = padDown;

        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        if (foregroundProcess == GetCurrentProcessId() && (kbToggle || padToggle))
            Toggle(player);

        if (!m_bEnabled && !m_bActive)
            return;

        if (m_bEnabled && !m_bActive)
        {
            Activate(player);
        }

        if (!m_bActive)
            return;

        m_activePlayer = player;

        if (Trigger::StpFlags.STP_OBJ) return;
        if (m_subweaponActive) return;
        UpdateUltimate(player, foregroundProcess == GetCurrentProcessId() && !gui::IsMenuVisible());
    }

    void PostTick(Pl0000* player)
    {
        if (!m_bActive || player != m_activePlayer || Trigger::StpFlags.STP_OBJ) return;
        if (m_subweaponActive)
        {
            m_electricCombat.Tick(player,false,false,false,false);
            if (m_stormLifetime && --m_stormLifetime == 0) StopThunderstorm();
            return;
        }
        UpdateRoundTrip(player);
        if (m_stormLifetime && --m_stormLifetime == 0) StopThunderstorm();
        const bool nativeAttack = SamUltimatePolicy::Attack(player->m_Rno0);
        const bool addonAttack = m_bBossEnderActive && m_currentAddon;
        m_electricCombat.Tick(player, nativeAttack || addonAttack,
            (nativeAttack && player->m_Rno0 >= 0x100012) ||
                (addonAttack && m_currentUltimate != 5 && m_currentUltimate != 6 &&
                    m_currentUltimate != 10 && m_currentUltimate != 11),
            m_bBossEnderActive && IsThunderMove(m_currentUltimate) && m_ultimateStage == 1,
            m_bBossEnderActive && (m_currentUltimate == 10 || m_currentUltimate == 11));
        SheathController::Instance().Update(player);
    }
};
