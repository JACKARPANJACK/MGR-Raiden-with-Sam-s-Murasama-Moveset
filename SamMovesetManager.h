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
#include "SamModelPatcher.h"
#include "SamNativeRuntime.h"
#include "SheathController.h"
#include "ChargeController.h"
#include "SamBossSequence.h"
#include "SamUltimatePolicy.h"
#include "gui.h"
#include "SamDirectionalPolicy.h"
#include "SamLightComboPolicy.h"
#include "SamNativeExecution.h"
#include "SamTemplateExecution.h"
#include "SamChargeFxPolicy.h"
#include "SamAttackInputPolicy.h"
#include "SamRoundTripPolicy.h"
#include "SamRockProjectile.h"
#include "SamElectricCombat.h"
#include "SamStagePolicy.h"
#include "EncounterPolicy.h"
#include "GunChargePolicy.h"
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
    // Requested is the user's Murasama selection. Active is only set after the DLC
    // archive has finished streaming and the routing has been installed.
    bool m_bEnabled = false;
    bool m_bActive = false;
    bool m_subweaponActive = false;
    unsigned m_groundAimTicks=0;
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
    SamDirectionalPolicy::CombatInput m_directionalInput;
    SamAttackInputPolicy::Reader m_attackInput;
    char m_nativeCombatClip[5]{};
    bool m_combatAllowed=false,m_combatFocused=false,m_combatSampled=false;
    unsigned m_preCombatAction=0;
    unsigned m_savedSprintOn=0,m_savedSprintTrig=0,m_savedPlayerSprintOn=0,m_savedPlayerSprintTrig=0;
    int m_pendingAddon = -1;
    unsigned m_addonTicks = 0;
    SamDirectionalPolicy::Direction m_pendingDirection = SamDirectionalPolicy::None;
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
    SamLightComboPolicy::Buffer m_lightCombo;
    SamExecutionPolicy::Counter m_parryExecution;
    EntityHandle m_executionVictim;
    int m_executionVictimHp = -1;
    EntityHandle m_counterVictim;
    int m_counterVictimHp = -1;
    unsigned m_finisherSelection = 0;
    bool m_rockSpawned = false;
    cEspControler* m_pStormEsp = nullptr;
    cEspControler* m_pAddonChargeEsp = nullptr;
    cEspControler* m_pAddonReleaseEsp = nullptr;
    SamChargeFxPolicy::State m_chargeFx;
    bool m_addonChargeActive = false;
    bool m_addonReleaseActive = false;
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

    bool m_storySuspended = false;
    static bool StoryEvent()
    { const auto* player=g_Scene.m_pPlayer;
      const bool combatQte=player && (SamExecutionPolicy::Native(player->m_Rno0) || player->m_Rno0==0x100059);
      return EncounterPolicy::Scripted(g_StaFlags.STA_EVENT, g_StaFlags.STA_QTE && !combatQte,
        g_StaFlags.STA_CODEC, g_StaFlags.STA_SOFT_EVENT); }
    bool IsEnabled() const { return m_bActive; }
    void UpdateSceneSafety(Pl0000* player)
    {
        if (!player || player != m_activePlayer || !m_bActive) return;
        if (StoryEvent())
        {
            m_subweaponActive=false;m_groundAimTicks=0;
            if (!m_storySuspended)
            {
                SamTemplateExecution::Get().Abort(player);
                if (m_roundTripActive) EndRoundTrip(player);
                ResetUltimate(); m_chargeController.Reset();
                m_electricCombat.Reset(); m_stormLifetime=0; StopThunderstorm();
                SheathController::Instance().SetSheathToHip(player,false);
                // Keep a story action the engine already selected. Only a Sam
                // owned action is reset when handing control back to Raiden.
                SamNativeRuntime::Get().Deactivate(player,SamTogglePolicy::OwnsAction(player->m_Rno0));
                g_GameStateManager.IsMainSamPlayer=false;
                g_GameStateManager.IsStyleChanged=false;
                m_storySuspended=true;
                Log("[SamMoveset] Story handoff: native Raiden graph, Sam selection retained");
            }
        }
        else if (m_storySuspended && !Trigger::StpFlags.STP_OBJ && !m_subweaponActive &&
            SamNativeRuntime::Get().Activate(player))
        {
            m_storySuspended=false;
            g_GameStateManager.IsMainSamPlayer=true;
            g_GameStateManager.IsStyleChanged=true;
            SheathController::Instance().SetSheathToHip(player,true);
            Log("[SamMoveset] Story handoff complete: Sam graph resumed");
        }
    }
    void SetSubweaponActive(bool active) { m_subweaponActive=active; }
    bool IsSubweaponActive() const { return m_subweaponActive; }
    void UpdateGroundSubweapon(Pl0000* player)
    {
        if(!player || player!=m_activePlayer || !m_bActive || !g_pPlayerManager ||
            StoryEvent() || m_storySuspended || Trigger::StpFlags.STP_OBJ) return;
        const int slot=g_pPlayerManager->getSubWeaponEquipped();
        const unsigned button=player->m_ButtonUseSubweapon&~0xFu;
        const bool held=(GetAsyncKeyState('C')&0x8000)!=0 ||
            ((g_dbPad.m_On|g_dbPad.m_Trig|player->m_CurrentInput.m_On|player->m_CurrentInput.m_Trig)&button)!=0;
        if(!m_subweaponActive)
        {
            if(!GunChargePolicy::GroundAimStart(slot,player->isInAir()!=FALSE,player->m_Rno0,held) ||
                !player->isAlive() || player->isBladeModeActive() || gui::IsMenuVisible() ||
                g_StaFlags.STA_PAUSE || m_roundTripActive || m_bBossEnderActive || !SamNativeRuntime::Get().Active(player)) return;
            DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
            if(foreground!=GetCurrentProcessId()) return;
            m_subweaponActive=true;m_groundAimTicks=0;
            // Restore the entire Raiden graph, map and battle parameters before
            // the native tick sees aim input. Animation fallback alone cannot
            // create Raiden's camera, reticle and secondary action states.
            SamNativeRuntime::Get().Deactivate(player,true);
            g_GameStateManager.IsMainSamPlayer=false;
            g_GameStateManager.IsStyleChanged=false;
            Log("[SamMoveset] Ground secondary aim: native Raiden graph slot=%d",slot);
            return;
        }
        ++m_groundAimTicks;
        // Native aiming/firing/recovery states must finish before restoring Sam.
        if(!player->isAlive() || !GunChargePolicy::GroundAimResume(player->m_Rno0,held,m_groundAimTicks)) return;
        if(SamNativeRuntime::Get().Activate(player))
        {
            m_subweaponActive=false;m_groundAimTicks=0;
            g_GameStateManager.IsMainSamPlayer=true;
            g_GameStateManager.IsStyleChanged=true;
            Log("[SamMoveset] Ground secondary recovery complete: Sam graph resumed");
        }
    }
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
        SamTemplateExecution::Get().Abort(player);
        SheathController::Instance().SetSheathToHip(player, false);
        if (m_roundTripActive)
            EndRoundTrip(player);
        m_electricCombat.Reset();
        m_chargeController.Reset();
        m_stormLifetime=0; StopThunderstorm();
        m_drawThunderstormActive=false; m_drawSlashStrikeTimer=0; m_wasChargingDraw=false;
        m_bActive = m_controllerInstalled = false;
        m_storySuspended = false;
        m_subweaponActive = false;
        m_activePlayer = nullptr;
        ResetUltimate();
        g_GameStateManager.IsMainSamPlayer = false;
        g_GameStateManager.IsStyleChanged = false;
        SamModelPatcher::Instance().ApplySamModels(false);
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
        out.bossEnderActive = m_bBossEnderActive || SamTemplateExecution::Get().Active(m_activePlayer);
        out.roundTripActive = m_roundTripActive;
        CopyText(out.bossEnderCode, sizeof(out.bossEnderCode), m_pBossEnderCode);
        if(SamTemplateExecution::Get().Active(m_activePlayer))
            CopyText(out.bossEnderCode,sizeof(out.bossEnderCode),SamTemplateExecution::Get().CurrentCode());
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
        if(!m_bBossEnderActive && SamArchiveLookup::ValidCode(code))std::memcpy(m_nativeCombatClip,code,5);
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
        if (!SamPlayerIdentity::Eligible(player) || m_bActive || StoryEvent() || Trigger::StpFlags.STP_OBJ ||
            !SamResourceManager::Instance().IsLoaded())
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

        // Switch model IDs to Sam's Murasama sheath and re-attach sheath to hip
        SamModelPatcher::Instance().ApplySamModels(true);
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
        if(m_activePlayer)SamTemplateExecution::Get().Abort(m_activePlayer);
        if (!SamPlayerIdentity::Eligible(player)) player = nullptr;
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
        m_storySuspended = false;
        m_subweaponActive = false;
        m_bEnabled = false;
        g_GameStateManager.IsMainSamPlayer = false;
        g_GameStateManager.IsStyleChanged = false;
        g_GameStateManager.InstantCharge = false;
        SamModelPatcher::Instance().ApplySamModels(false);
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
        if (index >= SamUltimatePolicy::Count()) return false;
        const char* name = SamUltimatePolicy::Moves[index].name.c_str();
        return (std::strstr(name, "thunder") != nullptr || std::strstr(name, "lightning") != nullptr);
    }

    void CycleUltimate()
    {
        ++m_bossEnderIndex;
    }

    const char* NextUltimateName() const
    {
        return SamUltimatePolicy::Count() > 0 ? SamUltimatePolicy::Moves[SamUltimatePolicy::UltimateIndex(m_bossEnderIndex)].name.c_str() : "";
    }
    bool UltimateQueued() const { return m_ultimateQueue.pending; }
    bool IsRoundTripActive() const { return m_roundTripActive; }
    void SetUltimateIndex(unsigned int index)
    {
        m_bossEnderIndex = SamUltimatePolicy::UltimateCount() ? index % SamUltimatePolicy::UltimateCount() : 0;
    }
    bool IsBossEnderActive() const { return m_bBossEnderActive; }
    void TriggerAddon(int addonIndex)
    {
        if (m_bActive && m_activePlayer && !m_roundTripActive && !m_bBossEnderActive &&
            addonIndex >= 0 && addonIndex < static_cast<int>(SamUltimatePolicy::Count()))
        {
            m_pendingAddon = addonIndex;
            m_addonTicks = 18;
            m_ultimateQueue.Reset();
        }
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
        m_pendingDirection = SamDirectionalPolicy::None;
        m_rtReleasePending = false;
        m_bBossEnderActive = false;
        m_bossEnderTicks = 0;
        m_electricCombat.EndGrab();
        m_ultimateStage = 0;
        m_pBossEnderCode = nullptr;
        m_lightCombo.Reset();
        m_directionalInput.Reset();
        m_parryExecution.Reset();
        m_executionVictim=nullptr;
        m_executionVictimHp=-1;
        m_counterVictim=nullptr;
        m_counterVictimHp=-1;
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
        if(m_addonReleaseActive && m_pAddonReleaseEsp)m_pAddonReleaseEsp->FadeUnits(2,0);
        m_addonReleaseActive=false;
        m_addonChargeActive=false;
        m_chargeFx.Reset();
        if(m_activePlayer)SheathController::Instance().SetChargePose(m_activePlayer,false);
    }

    bool ChargeFxReady(Pl0000* player)
    {
        return player && SamNativeRuntime::Get().Active(player) &&
            SamResourceManager::Instance().IsRuntimeReady() && SheathController::Instance().Ready(player) &&
            player->m_SwordHandle.getEntity() && player->m_SheathHandle.getEntity()==player->m_pSheathEntity;
    }
    void UpdateAddonCharge(Pl0000* player)
    {
        const bool ready=ChargeFxReady(player);
        if(m_chargeFx.Start(ready)) {
            if(!m_pAddonChargeEsp)m_pAddonChargeEsp=new cEspControler();
            m_addonChargeActive=SamNativeRuntime::Get().CreatePlayerEffect(player,117,m_pAddonChargeEsp);
        }
        // EST 117 contains the DLC charge buildup. EST 118 belongs to the
        // uncharged draw release (2102), not to a mature charge glow.
    }

    void* BossSequence(const char* code, void* sequence, bool raiden = false, bool playable = false)
    {
        const std::string key=std::string(raiden ? "pl0010:" : playable ? "pl1400:" : "em0020:")+code;
        auto it = m_bossSequences.find(key);
        if (it != m_bossSequences.end()) return it->second.data();
        auto& resources=SamResourceManager::Instance();
        auto copy = SamBossSequence::Adapt(sequence, raiden ? resources.GetRaidenSequenceSize(sequence) :
            resources.GetBossSequenceSize(sequence), code, raiden, playable);
        if (copy.empty()) return nullptr;
        auto inserted = m_bossSequences.emplace(key, std::move(copy));
        return inserted.first->second.data();
    }

    bool PlayBossStage(Pl0000* player, const char* code, bool chargeRelease=false)
    {
        const bool raiden=SamUltimatePolicy::Moves[m_currentUltimate].raiden;
        const bool playable=SamUltimatePolicy::Moves[m_currentUltimate].playable;
        const auto clip = raiden ? SamResourceManager::Instance().GetRaidenClip(code) : SamResourceManager::Instance().GetClip(code,
            playable ? SamArchiveLookup::Source::Playable : SamArchiveLookup::Source::Boss, false, true);
        if (!clip.motion || !clip.sequence) return false;
        void* sequence = BossSequence(code, clip.sequence, raiden, playable);
        if (!sequence) return false;
        m_stageLength = SamStagePolicy::Length(clip.motion);
        if (!m_stageLength || m_stageLength > 3600) return false;
        m_stageFrame = 0.0f;
        m_slamDone = m_lightningHit = false;
        SamNativeRuntime::Get().ConfigureDamage(player,true,SamBossSequence::HitCount(sequence),raiden);
        const int animation = player->setDirectAnimation(clip.motion, sequence, 0,
            0.05f, 1.0f, 0x8000000, 0.0f, SamBalancePolicy::AttackSpeed);
        if (animation == -1) return false;
        const bool windup=!raiden && SamChargeFxPolicy::Windup(code);
        if(!raiden) {
            SheathController::Instance().SetChargePose(player,windup,chargeRelease);
            SheathController::Instance().AnimateSheathDirect(player,code,SamBalancePolicy::AttackSpeed);
        }
        if(windup){m_chargeFx.Begin();UpdateAddonCharge(player);}
        else if(chargeRelease && !raiden && ChargeFxReady(player)) {
            if(!m_pAddonReleaseEsp)m_pAddonReleaseEsp=new cEspControler();
            m_pAddonReleaseEsp->FadeUnits(1,0);
            m_addonReleaseActive=SamNativeRuntime::Get().CreatePlayerEffect(player,119,m_pAddonReleaseEsp);
        }
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
        RecordAnimationMapRequest(-1, code, code, raiden ? "pl0010" : "em0020", true,
            raiden ? "Raiden ultimate: native motion, sequence and attack table" : "directional add-on: player-adapted boss sequence");
        RecordSequenceReplacement(code);
        Log("[SamMoveset] ULTIMATE stage=%s frames=%u boxes=%u name=%s", code,
            m_stageLength,SamBossSequence::HitCount(sequence),SamUltimatePolicy::Moves[m_currentUltimate].name);
        return true;
    }

    bool MoveReady(unsigned index)
    {
        if (index >= SamUltimatePolicy::Count()) return false;
        const auto& move = SamUltimatePolicy::Moves[index];
        if(!move.raiden && SamChargeFxPolicy::Windup(move.windup.c_str())) {
            if(m_activePlayer)SheathController::Instance().Update(m_activePlayer);
            if(!ChargeFxReady(m_activePlayer))return false;
        }
        for (const auto& code : move.Stages())
        {
            const auto clip = move.raiden ? SamResourceManager::Instance().GetRaidenClip(code.c_str()) :
                SamResourceManager::Instance().GetClip(code.c_str(), move.playable ? SamArchiveLookup::Source::Playable :
                    SamArchiveLookup::Source::Boss, false, true);
            if (!clip.motion || !clip.sequence || !SamStagePolicy::Length(clip.motion) ||
                !BossSequence(code.c_str(),clip.sequence,move.raiden,move.playable)) return false;
        }
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

    void FinishUltimate(Pl0000* player, bool completed=false)
    {
        const bool owns = player->m_Rno0 == SamUltimatePolicy::Action;
        const int continuation=completed && owns && !m_roundTripActive ? m_pendingAddon : -1;
        const auto continuationDirection=m_pendingDirection;
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
        if(continuation>=0 && owns && player->isAlive() && !player->isInAir()) {
            m_pendingAddon=continuation;
            m_pendingDirection=continuationDirection;
            m_addonTicks=18;
        }
        Log("[SamMoveset] ULTIMATE finished/cancelled");
    }

    void UpdateUltimate(Pl0000* player, bool focused)
    {
        if(SamTemplateExecution::Get().Active(player)) {
            // Latch blocked presses so holding a button during the execution
            // cannot become a fresh finisher/ultimate when the pair releases.
            m_xDown=(GetAsyncKeyState('X') & 0x8000)!=0;
            m_finisherDown=(GetAsyncKeyState('F') & 0x8000)!=0;
            m_previousUltimatePad=g_dbPad.m_On|g_dbPad.m_Trig;
            m_directionalInput.Reset();
            player->m_CurrentInput.m_Trig &= ~(player->m_ButtonLightAttack|player->m_ButtonHeavyAttack);
            RecordActionHandler("sam-template-execution");return;
        }
        const bool down = (GetAsyncKeyState('X') & 0x8000) != 0;
        const unsigned sprintButton=player->m_ButtonNinjarun&~0xFu;
        const bool sprinting=SamDirectionalPolicy::NativeSprint(player->m_Rno0,
            ((player->m_CurrentInput.m_On|g_dbPad.m_On)&sprintButton)!=0);
        const bool pressed = focused && !sprinting && SamUltimatePolicy::ReserveButton(player->m_Rno0,
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
        const auto attack=m_attackInput.Poll(player->m_CurrentInput.m_On|g_dbPad.m_On,
            player->m_CurrentInput.m_Trig|g_dbPad.m_Trig,player->m_ButtonLightAttack,player->m_ButtonHeavyAttack);
        const bool heavyHeld = attack.heavyHeld;
        m_chargeController.Tick(charging && heavyHeld, airborne);

        // Paired Sam sequences and Sam's native charge handler own charge FX.
        // Keep diagnostics separate from effect playback.
        float stickX = g_dbPad.m_LeftStick.x, stickY = g_dbPad.m_LeftStick.y;
        bool keyboardDirection = false, invalidDirection = false;
        if (focused)
        {
            const bool a=(GetAsyncKeyState('A') & 0x8000)!=0, d=(GetAsyncKeyState('D') & 0x8000)!=0;
            const bool w=(GetAsyncKeyState('W') & 0x8000)!=0, s=(GetAsyncKeyState('S') & 0x8000)!=0;
            keyboardDirection = a || d || w || s;
            invalidDirection = (a && d) || (w && s);
            if (keyboardDirection) { stickX=float(d)-float(a); stickY=float(w)-float(s); }
        }
        const bool lightTrig = attack.light;
        const bool heavyTrig = attack.heavy;
        const bool lightHeld = attack.lightHeld;
        if(lightTrig || heavyTrig)Log("[SamMoveset] INPUT action=%X pre=%X clip=%s light=%u heavy=%u heldH=%u",action,
            m_preCombatAction,m_nativeCombatClip,unsigned(lightTrig),unsigned(heavyTrig),unsigned(heavyHeld));
        const bool chord = (lightTrig && (heavyTrig || heavyHeld)) || (heavyTrig && lightHeld);
        const bool inputAllowed = focused && alive && !airborne && !sprinting && !m_roundTripActive &&
            !player->isBladeModeActive() && !g_StaFlags.STA_PAUSE && !invalidDirection &&
            (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action) || action == SamUltimatePolicy::Action);
        if (!inputAllowed || chord) m_directionalInput.Reset();
        const int addon = inputAllowed && !chord ? m_directionalInput.Poll(stickX, stickY,
            lightTrig, heavyTrig, !keyboardDirection) : -1;
        const auto currentDirection = SamDirectionalPolicy::Resolve(stickX, stickY);
        if ((lightTrig && m_pendingAddon!=SamLightComboPolicy::MoveIndex) || heavyTrig ||
            (m_pendingDirection != SamDirectionalPolicy::None && currentDirection != SamDirectionalPolicy::None &&
                currentDirection != m_pendingDirection) ||
            (currentDirection == SamDirectionalPolicy::None &&
                (std::fabs(stickX)>=0.25f || std::fabs(stickY)>=0.25f)))
        { m_pendingAddon = -1; m_pendingDirection = SamDirectionalPolicy::None; }
        const bool neutralLight = inputAllowed && !chord && lightTrig &&
            std::fabs(stickX)<0.25f && std::fabs(stickY)<0.25f && addon<0;
        const char* nativeClip = m_nativeCombatClip;
        const bool nativeEnder = !m_bBossEnderActive && SamUltimatePolicy::Attack(action) &&
            SamLightComboPolicy::NativeEnder(nativeClip);
        const bool bossLight = m_bBossEnderActive && m_currentUltimate == SamLightComboPolicy::MoveIndex;
        const bool completedEnder = m_lightCombo.pending && m_pendingAddon == SamLightComboPolicy::MoveIndex &&
            !m_bBossEnderActive && (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action));
        const bool heldLightContinuation=inputAllowed && !chord && lightHeld &&
            (nativeEnder || bossLight) && std::fabs(stickX)<0.25f && std::fabs(stickY)<0.25f;
        m_lightCombo.Poll(inputAllowed && (nativeEnder || completedEnder || bossLight), neutralLight || heldLightContinuation,
            heavyTrig || chord || addon >= 0 || invalidDirection ||
            std::fabs(stickX)>=0.25f || std::fabs(stickY)>=0.25f);
        if (nativeEnder && m_lightCombo.pending && MoveReady(SamLightComboPolicy::MoveIndex))
        {
            m_pendingAddon = SamLightComboPolicy::MoveIndex;
            m_pendingDirection = SamDirectionalPolicy::None;
            m_addonTicks = 18;
            m_ultimateQueue.Reset();
            player->m_CurrentInput.m_Trig &= ~player->m_ButtonLightAttack;
        }
        if (bossLight && neutralLight) player->m_CurrentInput.m_Trig &= ~player->m_ButtonLightAttack;
        if (inputAllowed &&
            ((!m_bBossEnderActive && (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action))) ||
             (m_bBossEnderActive && (m_directionalInput.chained || addon==7))) && addon >= 0 && MoveReady(addon))
        {
            m_pendingAddon = addon;
            m_pendingDirection = currentDirection;
            m_addonTicks = 360;
            m_ultimateQueue.Reset();
            player->m_CurrentInput.m_Trig &= ~(player->m_ButtonLightAttack | player->m_ButtonHeavyAttack);
        }
        if (finisherPressed && !sprinting && alive && !airborne && !m_roundTripActive && !m_bBossEnderActive &&
            (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action)) &&
            m_electricCombat.FinisherAvailable(player))
        {
            static constexpr unsigned finishers[]={10,1,3};
            m_pendingAddon = finishers[m_finisherSelection++ % 3];
            m_pendingDirection = SamDirectionalPolicy::None;
            m_addonTicks = 18;
            m_ultimateQueue.Reset();
        }
        // A string entered during a special waits for its authored stages to
        // finish. Damage/cancel paths discard it; completion transfers it.
        if (!m_bBossEnderActive && m_addonTicks && --m_addonTicks == 0) m_pendingAddon = -1;
        if (!inputAllowed || chord ||
            (m_pendingAddon == SamLightComboPolicy::MoveIndex && !m_lightCombo.pending) ||
            (!SamUltimatePolicy::Neutral(action) && !SamUltimatePolicy::Attack(action) && action != SamUltimatePolicy::Action))
            m_pendingAddon = -1;
        if (m_bBossEnderActive)
        {
            if (action != SamUltimatePolicy::Action)
            {
                ResetUltimate(); // damage/cutscene/other engine transition wins
                return;
            }
            if (airborne || !alive || player->isBladeModeActive() || ++m_bossEnderTicks > 600)
            {
                FinishUltimate(player);
                return;
            }
            m_stageFrame += SamBalancePolicy::AttackSpeed * (std::max)(0.0f,player->getSpeedRate());
            UpdateAddonCharge(player);
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
            const bool stageFinished=bossLight ?
                reinterpret_cast<float(__thiscall*)(Behavior*,int)>(shared::base+0x695BD0)(player,0)<=0.001f :
                SamStagePolicy::Finished(m_stageFrame,m_stageLength);
            const bool chargeWait=SamAttackInputPolicy::ChargeWait(m_chargeFx.active,heavyHeld,m_bossEnderTicks);
            if (stageFinished && !chargeWait)
            {
                const auto& activeMove = SamUltimatePolicy::Moves[m_currentUltimate];
                if (m_ultimateStage + 1 < activeMove.Stages().size() &&
                    (!bossLight || m_lightCombo.pending))
                {
                    ++m_ultimateStage;
                    m_lightCombo.Reset();
                    const bool chargeRelease=m_chargeFx.active;
                    StopAddonCharge();
                    if (!PlayBossStage(player, activeMove.Stage(m_ultimateStage).c_str(),chargeRelease))
                        FinishUltimate(player);
                }
                else FinishUltimate(player,true);
            }
            return;
        }

        if(sprinting)m_ultimateQueue.Reset();
        m_ultimateQueue.Tick(action, alive, airborne);
        if (!alive || airborne) { m_lightCombo.Reset(); return; }
        if (!m_roundTripActive && pressed && (SamUltimatePolicy::Neutral(action) || SamUltimatePolicy::Attack(action)))
        {
            m_ultimateQueue.Request();
            Log("[SamMoveset] ULTIMATE queued by X/controller B: %s", NextUltimateName());
        }
        if (m_roundTripActive || (!m_ultimateQueue.pending && m_pendingAddon < 0)) return;
        const bool finished = g_GameFunctionManager.IsAnimationEnded(player, 0);
        const bool restartedLight=m_pendingAddon==SamLightComboPolicy::MoveIndex && m_lightCombo.pending &&
            !std::strcmp(m_nativeCombatClip,"2000");
        const bool freshAttack=restartedLight ||
            (addon>=0 && (lightTrig || heavyTrig) && SamUltimatePolicy::Neutral(m_preCombatAction));
        if (!SamUltimatePolicy::CanStart(action, alive, airborne, finished,freshAttack)) return;
        m_currentAddon = m_pendingAddon >= 0;
        m_currentUltimate = m_currentAddon ? static_cast<unsigned>(m_pendingAddon) :
            SamUltimatePolicy::UltimateIndex(m_bossEnderIndex);
        const auto& move = SamUltimatePolicy::Moves[m_currentUltimate];
        // Preflight every stage before taking ownership of combat dispatch.
        if (!MoveReady(m_currentUltimate))
        {
            m_ultimateQueue.Reset();
            m_pendingAddon = -1;
            Log("[SamMoveset] ULTIMATE unavailable: incomplete move %s", move.name.c_str());
            return;
        }
        m_ultimateQueue.Reset();
        m_pendingAddon = -1;
        m_bBossEnderActive = true;
        m_ultimateStage = 0;
        m_lightCombo.Reset();
        m_executionVictim = SamExecutionPolicy::Finisher(m_currentUltimate) ? SamTargets::Find(player,3.0f,true) : nullptr;
        Entity* executionTarget=m_executionVictim.getEntity();
        m_executionVictimHp=executionTarget && executionTarget->m_pBehavior ?
            static_cast<BehaviorAppBase*>(executionTarget->m_pBehavior)->m_Hp : -1;
        // A ready finishing victim enters the template pair before any addon
        // hitbox can kill it with ordinary damage and discard the pairing.
        if (executionTarget && SamTemplateExecution::Get().Start(player,executionTarget,
            SamTemplateExecutionPolicy::ForMove(m_currentUltimate)))
        {
            m_electricCombat.ReleaseTarget(executionTarget);
            ResetUltimate();
            player->m_CurrentInput.m_Trig &= ~(player->m_ButtonLightAttack | player->m_ButtonHeavyAttack);
            Log("[SamMoveset] Finisher started using campaign-template attacker/victim pair");
            return;
        }
        player->m_CurrentInput.m_Trig &= ~(player->m_ButtonLightAttack | player->m_ButtonHeavyAttack);
        player->setRno(SamUltimatePolicy::Action, 0, 0, 0);
        if (!PlayBossStage(player, !move.windup.empty() ? move.windup.c_str() : move.release.c_str())) FinishUltimate(player);
        else if (!m_currentAddon) ++m_bossEnderIndex;
    }

    void SetEnabled(Pl0000* player,bool enabled)
    {
        if (!SamPlayerIdentity::Eligible(player)) return;
        if (!enabled)
        {
            if (m_bEnabled || m_bActive) Deactivate(player);
            return;
        }
        if (m_bEnabled || m_bActive) return;
        if (SamExecutionPolicy::Native(player->m_Rno0) || SamTemplateExecution::Get().Active(player)) return;

        m_bEnabled = true;
        Log("[SamMoveset] Murasama selection ON requested");
        SamResourceManager::Instance().RequestAllSamResources();
        SamResourceManager::Instance().Update();
        Activate(player);
    }

    void OnTick(Pl0000* player)
    {
        m_combatAllowed=false;m_combatSampled=false;
        if (!player || reinterpret_cast<uintptr_t>(player) < 0x10000) return;
        if (!player->m_pEntity || reinterpret_cast<uintptr_t>(player->m_pEntity) < 0x10000) return;
        if (!SamPlayerIdentity::Eligible(player)) return;

        if (m_activePlayer && m_activePlayer != player)
            Deactivate(m_activePlayer);

        // Service DLC resource streaming every tick
        SamResourceManager::Instance().Update();

        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        if (!m_bEnabled && !m_bActive)
            return;

        if (m_bEnabled && !m_bActive)
        {
            Activate(player);
        }

        if (!m_bActive)
            return;

        m_activePlayer = player;

        if (Trigger::StpFlags.STP_OBJ || m_subweaponActive || m_storySuspended || StoryEvent())
        {
            m_directionalInput.Reset();
            m_lightCombo.Reset();
            m_pendingAddon = -1;
            m_pendingDirection = SamDirectionalPolicy::None;
            return;
        }
        m_combatAllowed=true;
        m_preCombatAction=player->m_Rno0;
        m_combatFocused=foregroundProcess == GetCurrentProcessId() && !gui::IsMenuVisible();
        if(m_bBossEnderActive) {
            const unsigned run=player->m_ButtonNinjarun&~0xFu;
            m_savedSprintOn=g_dbPad.m_On&run;m_savedSprintTrig=g_dbPad.m_Trig&run;
            m_savedPlayerSprintOn=player->m_CurrentInput.m_On&run;
            m_savedPlayerSprintTrig=player->m_CurrentInput.m_Trig&run;
            g_dbPad.m_On&=~run;g_dbPad.m_Trig&=~run;
            player->m_CurrentInput.m_On&=~run;player->m_CurrentInput.m_Trig&=~run;
        }
    }

    void RestoreCombatInputs()
    {
        g_dbPad.m_On|=m_savedSprintOn;g_dbPad.m_Trig|=m_savedSprintTrig;
        if(m_activePlayer) {
            m_activePlayer->m_CurrentInput.m_On|=m_savedPlayerSprintOn;
            m_activePlayer->m_CurrentInput.m_Trig|=m_savedPlayerSprintTrig;
        }
        m_savedSprintOn=m_savedSprintTrig=m_savedPlayerSprintOn=m_savedPlayerSprintTrig=0;
    }
    void SampleCombatInputs(Pl0000* player,bool nativeDispatch=false)
    {
        if(!m_combatAllowed || m_combatSampled || !m_bActive || player!=m_activePlayer)return;
        m_combatSampled=true;UpdateUltimate(player,m_combatFocused);
        if(nativeDispatch && m_bBossEnderActive) {
            const unsigned run=player->m_ButtonNinjarun&~0xFu;
            m_savedSprintOn|=g_dbPad.m_On&run;m_savedSprintTrig|=g_dbPad.m_Trig&run;
            m_savedPlayerSprintOn|=player->m_CurrentInput.m_On&run;
            m_savedPlayerSprintTrig|=player->m_CurrentInput.m_Trig&run;
            g_dbPad.m_On&=~run;g_dbPad.m_Trig&=~run;
            player->m_CurrentInput.m_On&=~run;player->m_CurrentInput.m_Trig&=~run;
        }
    }

    void PostTick(Pl0000* player)
    {
        SampleCombatInputs(player); // fallback for native states that bypass action dispatch
        if (!m_bActive || player != m_activePlayer || Trigger::StpFlags.STP_OBJ ||
            m_storySuspended || StoryEvent()) return;
        if(SamTemplateExecution::Get().Active(player)) {
            SamTemplateExecution::Get().Tick(player);
            SheathController::Instance().Update(player);
            return;
        }
        if (m_subweaponActive)
        {
            m_electricCombat.Tick(player,false,false,false,false);
            if (m_stormLifetime && --m_stormLifetime == 0) StopThunderstorm();
            return;
        }
        Entity* executionTarget = m_bBossEnderActive ? m_executionVictim.getEntity() :
            player->field_2C1C.getEntity(); // the native counter attacker, never a nearby bystander
        const bool ownedHit=SamNativeExecution::OwnedHit(player,executionTarget);
        const int counterHp=executionTarget && executionTarget->m_pBehavior ?
            static_cast<BehaviorAppBase*>(executionTarget->m_pBehavior)->m_Hp : -1;
        const bool counterHit=ownedHit && executionTarget==m_counterVictim.getEntity() &&
            m_counterVictimHp>counterHp && counterHp>0;
        m_counterVictim=executionTarget;
        m_counterVictimHp=counterHp;
        bool finisherHit=false;
        if (m_bBossEnderActive && executionTarget && executionTarget->m_pBehavior &&
            SamExecutionPolicy::Finisher(m_currentUltimate))
        {
            const int hp=static_cast<BehaviorAppBase*>(executionTarget->m_pBehavior)->m_Hp;
            finisherHit=ownedHit && m_executionVictimHp>hp && hp>0;
            m_executionVictimHp=hp;
        }
        const bool parryExecute=m_parryExecution.Poll(player->m_Rno0,counterHit);
        if ((finisherHit || parryExecute) && SamTemplateExecution::Get().Start(player,executionTarget,
            parryExecute ? SamTemplateExecutionPolicy::Route::Counter : SamTemplateExecutionPolicy::ForMove(m_currentUltimate)))
        {
            m_electricCombat.ReleaseTarget(executionTarget);
            ResetUltimate();
            Log("[SamMoveset] Campaign-template paired execution started on confirmed hit");
            return;
        }
        UpdateRoundTrip(player);
        if (m_stormLifetime && --m_stormLifetime == 0) StopThunderstorm();
        const bool nativeAttack = SamUltimatePolicy::Attack(player->m_Rno0);
        const bool addonAttack = m_bBossEnderActive && m_currentAddon;
        m_electricCombat.Tick(player, nativeAttack || addonAttack,
            (nativeAttack && player->m_Rno0 >= 0x100012) ||
                (addonAttack && m_currentUltimate != 5 && m_currentUltimate != 6 &&
                    m_currentUltimate != 10 && m_currentUltimate != 11 &&
                    m_currentUltimate != SamLightComboPolicy::MoveIndex),
            m_bBossEnderActive && IsThunderMove(m_currentUltimate) && m_ultimateStage == 1,
            m_bBossEnderActive && (m_currentUltimate == 10 || m_currentUltimate == 11));
        SheathController::Instance().Update(player);
    }
};
