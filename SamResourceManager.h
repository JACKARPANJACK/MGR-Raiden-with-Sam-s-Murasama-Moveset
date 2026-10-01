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
    struct RequestState { unsigned int id; bool requested; bool pinned; };
    RequestState m_requests[5] = {
        {0x11400, false, false}, {0x11403, false, false},
        {0x11404, false, false}, {0x20020, false, false}, {0x3C001, false, false}
    };
    bool m_isLoaded = false;
    bool m_effectsRegistered = false;
    bool m_requestIssued = false;
    bool m_dlcMounted = false;
    std::vector<uint8_t> m_chargeRecoverySequence;

    SamResourceManager() = default;


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
            // Requests are retained for process lifetime: animation slots may
            // still reference these archives after G switches off.
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
                if (g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x20020), 0))
                    g_ObjReadManager.getDataAtSet(m_em0020Fmerge, static_cast<eObjID>(0x20020), 0);
                if (g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(0x11404), 0))
                    g_ObjReadManager.getDataAtSet(m_pl1404Fmerge, static_cast<eObjID>(0x11404), 0);

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
        if (!m_isLoaded) return false;
        for (const auto& resource : m_requests)
            if (!resource.requested || !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(resource.id), 0))
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
        return 0;
    }

    bool IsLoaded() const { return m_isLoaded; }
    bool IsRuntimeReady()
    {
        if (!m_isLoaded) return false;
        return RegisterEffectsAndSound();
    }
    Hw::cFmerge* GetFmerge() { return m_isLoaded ? &m_pl1400Fmerge : nullptr; }
    size_t GetPl1400FileCount() { return m_isLoaded ? m_pl1400Fmerge.getFileAmount() : 0; }
    size_t GetEm0020FileCount() { return m_isLoaded ? m_em0020Fmerge.getFileAmount() : 0; }
};
