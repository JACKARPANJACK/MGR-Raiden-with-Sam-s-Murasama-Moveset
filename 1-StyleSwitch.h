#pragma once

#include <Pl0000.h>
#include <cstring>
#include <vector>
#include <windows.h>
#include "SamAnimationPairs.h"

class StyleSwitch
{
public:
    using RuntimeClipResolver = bool(*)(const char* code, bool requireSequence);

private:
    inline static RuntimeClipResolver s_runtimeClipResolver = nullptr;

    struct ModifiedUnit
    {
        int animId;
        char originalName[4];
        char samName[4];
    };

    std::vector<ModifiedUnit> m_modifiedUnits;
    bool m_bActive = false;
    AnimationMap* m_pAppliedMap = nullptr;

    static void WriteName(char* dest, const char* src)
    {
        DWORD oldProtect = 0;
        if (VirtualProtect(dest, 4, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            std::memcpy(dest, src, 4);
            VirtualProtect(dest, 4, oldProtect, &oldProtect);
        }
        else
        {
            std::memcpy(dest, src, 4);
        }
    }

public:
    StyleSwitch() = default;

    static StyleSwitch& GetInstance()
    {
        static StyleSwitch instance;
        return instance;
    }

    static void SetRuntimeClipResolver(RuntimeClipResolver resolver)
    {
        s_runtimeClipResolver = resolver;
    }

    ~StyleSwitch()
    {
        Reset();
    }

    void Reset()
    {
        if (!m_bActive || !m_pAppliedMap)
        {
            m_bActive = false;
            m_modifiedUnits.clear();
            m_pAppliedMap = nullptr;
            return;
        }

        __try
        {
            auto* units = m_pAppliedMap->m_pUnits;
            if (units && units->m_pArray && units->m_Size > 0)
            {
                for (const auto& mod : m_modifiedUnits)
                {
                    for (int i = 0; i < units->m_Size; ++i)
                    {
                        if (units->m_pArray[i].m_Id == mod.animId)
                        {
                            WriteName(units->m_pArray[i].m_pName, mod.originalName);
                            break;
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        m_bActive = false;
        m_modifiedUnits.clear();
        m_pAppliedMap = nullptr;
    }

    void SetStyle(Pl0000* player, bool enable)
    {
        if (!player || !player->m_pAnimationMap)
            return;

        AnimationMap* map = player->m_pAnimationMap;

        if (!enable)
        {
            Reset();
            return;
        }

        if (m_bActive && m_pAppliedMap == map)
            return;

        // If map pointer changed (e.g. reload), restore old before applying new
        if (m_bActive)
            Reset();

        auto* units = map->m_pUnits;
        if (!units || !units->m_pArray || units->m_Size <= 0 || units->m_Size > 8192)
            return;

        m_pAppliedMap = map;
        m_modifiedUnits.clear();

        for (int i = 0; i < units->m_Size; ++i)
        {
            auto& unit = units->m_pArray[i];
            for (const auto& pair : kSamAnimationPairs)
            {
                if (std::memcmp(unit.m_pName, pair.from, 4) != 0)
                    continue;

                if (s_runtimeClipResolver && !s_runtimeClipResolver(pair.to, false))
                    break;

                ModifiedUnit mod;
                mod.animId = unit.m_Id;
                std::memcpy(mod.originalName, unit.m_pName, 4);
                std::memcpy(mod.samName, pair.to, 4);
                m_modifiedUnits.push_back(mod);

                WriteName(unit.m_pName, pair.to);
                break;
            }
        }

        m_bActive = true;
    }

    size_t GetModifiedCount() const { return m_modifiedUnits.size(); }

    void Update(Pl0000* player)
    {
        if (!player || !player->m_pAnimationMap || !m_bActive)
            return;

        // If player respawned with a new AnimationMap, reapply
        if (player->m_pAnimationMap != m_pAppliedMap)
        {
            SetStyle(player, true);
        }
    }

    StyleSwitch(const StyleSwitch&) = delete;
    StyleSwitch& operator=(const StyleSwitch&) = delete;
    StyleSwitch(StyleSwitch&&) = delete;
    StyleSwitch& operator=(StyleSwitch&&) = delete;
};

inline StyleSwitch& g_StyleSwitch = StyleSwitch::GetInstance();
