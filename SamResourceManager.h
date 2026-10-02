#pragma once

#include <shared.h>
#include <Hw.h>
#include <cObjReadManager.h>
#include <eObjID.h>
#include "SamArchiveLookup.h"
#include "SamSheathPolicy.h"
#include "SamBossSequence.h"
#include <windows.h>
#include <cstdio>
#include <initializer_list>
#include <vector>

class SamResourceManager
{
private:
    Hw::cFmerge m_pl1400Fmerge;
    Hw::cFmerge m_pl1404Fmerge;
    Hw::cFmerge m_em0020Fmerge;
    Hw::cFmerge m_wolfFmerge;
    Hw::cFmerge m_heatbladeFmerge;
    Hw::cFmerge m_raidenFmerge;
    Hw::cFmerge m_grenadeFmerge;
    bool m_grenadeBank=false;
    bool m_samBank = false, m_bossBank = false, m_wolfBank = false, m_heatbladeBank = false;
    bool m_sheathBank = false;
    struct RequestState { unsigned int id; bool requested; bool pinned; };
    RequestState m_requests[9] = {
        {0x11400, false, false}, {0x11403, false, false},
        {0x11404, false, false}, {0x20020, false, false}, {0x3C001, false, false},
        {0x20220, false, false}, {0x30372, false, false}, {0x10010, false, false}, {0x31011,false,false}
    };
    bool m_isLoaded = false;
    bool m_effectsRegistered = false;
    bool m_requestIssued = false;
    bool m_dlcMounted = false;
    std::vector<uint8_t> m_chargeRecoverySequence;

    SamResourceManager() = default;

    static bool RegisterBank(unsigned id, Hw::cFmerge& archive)
    {
        if (!g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(id), 0) ||
            !g_ObjReadManager.getDataAtSet(archive, static_cast<eObjID>(id), 0)) return false;
        // Native EFF/EFT registration: extension lookup, bank alias resolution
        // and duplicate/resource checks are all handled by the engine.
        using Register = BOOL(__cdecl*)(unsigned, Hw::cFmerge*);
        return reinterpret_cast<Register>(shared::base + 0xA00C50)(id, &archive) != FALSE;
    }


public:
    static SamResourceManager& Instance()
    {
        static SamResourceManager s_instance;
        return s_instance;
    }

    // Mount DLC CPK (data107.cpk) into the game engine's file system
    void MountSamDlc()
    {
        if (m_dlcMounted) return;
        __try
        {
            typedef void (__cdecl* MountDlcFn)();
            MountDlcFn mountDlc = reinterpret_cast<MountDlcFn>(shared::base + 0x5C73D0);
            mountDlc();
            m_dlcMounted = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // Issue streaming requests for all Sam campaign resources from data107.cpk
    void RequestAllSamResources()
    {
        // 1. Ensure DLC CPK is mounted
        MountSamDlc();

        if (!m_dlcMounted || m_requestIssued) return;

        // 2. Request all Sam object IDs from the engine CPK archive
        __try
        {
            // Retain requests through G toggles and story suspension. They are
            // balanced only after the native player has finished shutdown.
            bool accepted = true;
            for (auto& resource : m_requests)
            {
                if (!resource.requested)
                    resource.requested = g_ObjReadManager.requestObject(static_cast<eObjID>(resource.id), 0) != FALSE;
                accepted = resource.requested && accepted;
            }
            m_requestIssued = accepted;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Effect initialization must wait for archive completion.
    }

    // Called on tick to catch background CPK completion and extract the archive container
    void Update()
    {
        if (!m_requestIssued)
            RequestAllSamResources();

        __try
        {
            if (!m_isLoaded)
            {
                if (g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x11400), 0) != FALSE)
                {
                    if (g_ObjReadManager.getDataAtSet(m_pl1400Fmerge, static_cast<eObjID>(0x11400), 0))
                    {
                        m_isLoaded = true;
                    }
                }
            }

            if (m_isLoaded)
            {
                if (!m_samBank) m_samBank = RegisterBank(0x11400, m_pl1400Fmerge);
                if (g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x11404), 0) &&
                    !m_sheathBank)
                {
                    g_ObjReadManager.getDataAtSet(m_pl1404Fmerge, static_cast<eObjID>(0x11404), 0);
                    m_sheathBank = RegisterBank(0x11404, m_pl1404Fmerge);
                }
                if (!m_bossBank) m_bossBank = RegisterBank(0x20020, m_em0020Fmerge);
                if (!m_wolfBank) m_wolfBank = RegisterBank(0x20220, m_wolfFmerge);
                if (!m_heatbladeBank) m_heatbladeBank = RegisterBank(0x30372, m_heatbladeFmerge);
                if (!m_grenadeBank) m_grenadeBank = RegisterBank(0x31011,m_grenadeFmerge);
                if (!m_raidenFmerge.getFileAmount() && g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x10010), 0))
                    g_ObjReadManager.getDataAtSet(m_raidenFmerge, static_cast<eObjID>(0x10010), 0);

                RegisterEffectsAndSound();

                for (auto& resource : m_requests)
                {
                    if (!resource.pinned && resource.requested &&
                        g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(resource.id), 0))
                    {
                        g_ObjReadManager.addUseRef(static_cast<eObjID>(resource.id), 0);
                        resource.pinned = true;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    bool RegisterEffectsAndSound()
    {
        if (m_effectsRegistered)
            return true;
        if (!m_isLoaded || !m_samBank) return false;
        for (const auto& resource : m_requests)
            if (resource.id == 0x11400 && (!resource.requested ||
                !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(resource.id), 0)))
                return false;

        __try
        {
            // Execute the engine's native DLC 1 initialization function at shared::base + 0x6538D0
            // Setting DLC mode flag at shared::base + 0x17EA030 to 8 triggers the full native routine:
            // - Registers EffectAttr.bxm & pl1400.eff into global effect table
            // - Registers SeAttr.bxm & Sam's sound bank (pl1400_se_...)
            // - Registers EffectBullet.bxm
            int* pDlcMode = reinterpret_cast<int*>(shared::base + 0x17EA030);
            if (pDlcMode)
            {
                const int originalMode = *pDlcMode;
                // Native DLC initialization also selects DLC sound, collision-
                // effect and effect-bullet tables globally. Restoring only DLC
                // mode left bosses, enemies and cutscenes using Sam's tables.
                // Register the DLC layer, then restore the scene's selectors.
                constexpr unsigned selectors[]={0x1778858,0x17781B8,0x19BD180,0x19BD184,
                    0x148F5DC,0x148F5E0,0x148F5E4};
                unsigned saved[7]{};
                for (unsigned i=0;i<7;++i) saved[i]=*reinterpret_cast<unsigned*>(shared::base+selectors[i]);
                __try
                {
                    *pDlcMode = 8;
                    using DlcInitFn = void(__cdecl*)();
                    reinterpret_cast<DlcInitFn>(shared::base + 0x6538D0)();
                    m_effectsRegistered = true;
                }
                __finally
                {
                    *pDlcMode = originalMode;
                    for (unsigned i=0;i<7;++i)
                        *reinterpret_cast<unsigned*>(shared::base+selectors[i])=saved[i];
                }
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        return false;
    }

    SamArchiveLookup::Clip GetClip(const char* code,
        SamArchiveLookup::Source source = SamArchiveLookup::Source::Playable,
        bool allowFallback = true, bool requireSequence = true)
    {
        if (!m_isLoaded) return {};
        auto clip = SamArchiveLookup::Resolve(code, source, allowFallback, requireSequence,
            [this](SamArchiveLookup::Source archive, const char* name) -> void*
            {
                return (archive == SamArchiveLookup::Source::Playable ?
                    m_pl1400Fmerge : m_em0020Fmerge).getFileNameData(name);
            });
        if (clip.sequence && clip.source == SamArchiveLookup::Source::Playable && !std::strcmp(code,"242b"))
        {
            if (m_chargeRecoverySequence.empty())
                for (size_t i=0;i<m_pl1400Fmerge.getFileAmount();++i)
                    if (m_pl1400Fmerge.getFileIndexData(i)==clip.sequence)
                    {
                        m_chargeRecoverySequence=SamBossSequence::RepairPlayableEffects(clip.sequence,m_pl1400Fmerge.getFileIndexSize(i));
                        break;
                    }
            if (!m_chargeRecoverySequence.empty()) clip.sequence=m_chargeRecoverySequence.data();
        }
        return clip;
    }

    SamArchiveLookup::Clip GetRaidenClip(const char* code)
    {
        if (!m_isLoaded || !SamArchiveLookup::ValidCode(code) ||
            !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x10010),0)) return {};
        char filename[64]{};
        std::snprintf(filename,sizeof(filename),"pl0010_%s.mot",code);
        void* motion=m_raidenFmerge.getFileNameData(filename);
        std::snprintf(filename,sizeof(filename),"pl0010_%s_0_seq.bxm",code);
        void* sequence=m_raidenFmerge.getFileNameData(filename);
        return motion && sequence ? SamArchiveLookup::Clip{motion,sequence} : SamArchiveLookup::Clip{};
    }
    size_t GetRaidenSequenceSize(void* data)
    {
        if (!data || !m_isLoaded) return 0;
        for (size_t i=0;i<m_raidenFmerge.getFileAmount();++i)
            if (m_raidenFmerge.getFileIndexData(i)==data) return m_raidenFmerge.getFileIndexSize(i);
        return 0;
    }
    void* GetMotion(const char* code) { return GetClip(code).motion; }
    void* GetSequence(const char* code) { return GetClip(code).sequence; }

    void* GetSheathMotion(const char* code)
    {
        if (!m_isLoaded || !code || !*code) return nullptr;

        char cleanCode[16]{};
        if (!SamSheathPolicy::Code(cleanCode, code)) return nullptr;
        const char* mapped = SamSheathPolicy::MotionCode(cleanCode);
        if (mapped != cleanCode) std::memcpy(cleanCode, mapped, 4);

        char filename[64]{};
        std::snprintf(filename, sizeof(filename), "pl1404_%s.mot", cleanCode);
        void* result = m_pl1404Fmerge.getFileNameData(filename);
        if (!result)
        {
            std::snprintf(filename, sizeof(filename), "Pl1404_%s.mot", cleanCode);
            result = m_pl1404Fmerge.getFileNameData(filename);
        }
        if (!result)
        {
            // Locomotion and combat fallbacks
            const char* fallback = nullptr;
            if (!std::strcmp(cleanCode, "0001") || !std::strcmp(cleanCode, "0002") || !std::strcmp(cleanCode, "0003"))
                fallback = "0000";
            else if (!std::strcmp(cleanCode, "0202") || !std::strcmp(cleanCode, "0203"))
                fallback = "0200";
            else if (!std::strcmp(cleanCode, "0230") || !std::strcmp(cleanCode, "0231") || !std::strcmp(cleanCode, "0232"))
                fallback = "023a";
            else if (!std::strcmp(cleanCode, "3003") || !std::strcmp(cleanCode, "3004"))
                fallback = "3010";
            else if (!std::strcmp(cleanCode, "9100") || !std::strcmp(cleanCode, "9101") || !std::strcmp(cleanCode, "9102"))
                fallback = "9108";
            else if (!std::strcmp(cleanCode, "92e0"))
                fallback = "2250";
            else if (!std::strcmp(cleanCode, "92e4"))
                fallback = "2253";

            if (fallback)
            {
                std::snprintf(filename, sizeof(filename), "pl1404_%s.mot", fallback);
                result = m_pl1404Fmerge.getFileNameData(filename);
                if (!result)
                {
                    std::snprintf(filename, sizeof(filename), "Pl1404_%s.mot", fallback);
                    result = m_pl1404Fmerge.getFileNameData(filename);
                }
            }
        }
        if (!result)
        {
            std::snprintf(filename, sizeof(filename), "pl1400_%s.mot", cleanCode);
            result = m_pl1404Fmerge.getFileNameData(filename);
        }
        return result;
    }

    void* GetRawFile(const char* filename,
        SamArchiveLookup::Source source = SamArchiveLookup::Source::Playable)
    {
        if (!m_isLoaded || !filename) return nullptr;
        return (source == SamArchiveLookup::Source::Playable ?
            m_pl1400Fmerge : m_em0020Fmerge).getFileNameData(filename);
    }

    size_t GetBossSequenceSize(void* data)
    {
        if (!data || !m_isLoaded) return 0;
        for (size_t i = 0, count = m_em0020Fmerge.getFileAmount(); i < count; ++i)
            if (m_em0020Fmerge.getFileIndexData(i) == data)
                return m_em0020Fmerge.getFileIndexSize(i);
        for (size_t i = 0, count = m_pl1400Fmerge.getFileAmount(); i < count; ++i)
            if (m_pl1400Fmerge.getFileIndexData(i) == data)
                return m_pl1400Fmerge.getFileIndexSize(i);
        return SamBossSequence::Size(data);
    }

    bool IsLoaded() const { return m_isLoaded; }
    bool HeatbladesReady() const { return m_wolfBank && m_heatbladeBank; }
    bool ExplosivesReady() const { return m_grenadeBank; }
    void SceneReleased()
    {
        using ReleaseBank=void(__cdecl*)(unsigned,Hw::cFmerge*);
        auto release=reinterpret_cast<ReleaseBank>(shared::base+0xA00D60);
        if (m_samBank) release(0x11400,&m_pl1400Fmerge);
        if (m_bossBank) release(0x20020,&m_em0020Fmerge);
        if (m_wolfBank) release(0x20220,&m_wolfFmerge);
        if (m_heatbladeBank) release(0x30372,&m_heatbladeFmerge);
        if (m_grenadeBank) release(0x31011,&m_grenadeFmerge);
        // Player shutdown has completed. Balance only this plugin's requests;
        // enemies and effect work retain their own native references.
        for (auto& resource : m_requests)
        {
            if (resource.pinned) g_ObjReadManager.removeReference(static_cast<eObjID>(resource.id),0);
            if (resource.requested) g_ObjReadManager.removeRequest(static_cast<eObjID>(resource.id),0);
            resource.requested = resource.pinned = false;
        }
        m_isLoaded = m_effectsRegistered = m_requestIssued = false;
        m_samBank = m_bossBank = m_wolfBank = m_heatbladeBank = false;
        m_sheathBank = false;
        m_grenadeBank=false;
        // Sequence copies are immutable and may still be bound during cleanup.
        // Reacquire archive pointers from the next scene before using them.
    }
    bool IsRuntimeReady()
    {
        if (!m_isLoaded) return false;
        return RegisterEffectsAndSound();
    }
    Hw::cFmerge* GetFmerge() { return m_isLoaded ? &m_pl1400Fmerge : nullptr; }
    size_t GetPl1400FileCount() { return m_isLoaded ? m_pl1400Fmerge.getFileAmount() : 0; }
    size_t GetEm0020FileCount() { return m_isLoaded ? m_em0020Fmerge.getFileAmount() : 0; }
};
