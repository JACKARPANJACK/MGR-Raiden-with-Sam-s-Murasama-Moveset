#pragma once
#include "1-GlobleVarious.h"
#include <mutex>
#include <cstdint>

class SamModelPatcher
{
private:
    static constexpr uint32_t COSTUME_COUNT = 15;
    mutable std::mutex m_mutex;
    bool m_isSamPatched = false;
    uint32_t m_originalSheaths[COSTUME_COUNT]{};
    bool m_hasOriginals = false;

    static uintptr_t GetSheathAddr(uint32_t index)
    {
        return shared::base + 0x14A9830 + (index * 0x14);
    }

public:
    static SamModelPatcher& Instance()
    {
        static SamModelPatcher s_instance;
        return s_instance;
    }

    SamModelPatcher() = default;

    void ApplySamModels(bool isSam)
    {
        std::lock_guard lock(m_mutex);
        if (m_isSamPatched == isSam) return;

        if (isSam)
        {
            if (!m_hasOriginals)
            {
                for (uint32_t i = 0; i < COSTUME_COUNT; ++i)
                {
                    uintptr_t addr = GetSheathAddr(i);
                    m_originalSheaths[i] = *reinterpret_cast<const uint32_t*>(addr);
                }
                m_hasOriginals = true;
            }

            // Patch all 15 costume sheath model IDs with Sam's Murasama sheath (0x11404)
            for (uint32_t i = 0; i < COSTUME_COUNT; ++i)
            {
                uintptr_t addr = GetSheathAddr(i);
                injector::WriteMemory<uint32_t>(addr, 0x11404u, true);
            }
            m_isSamPatched = true;
        }
        else
        {
            // Fully restore original sheath IDs back to all 15 costumes
            if (m_hasOriginals)
            {
                for (uint32_t i = 0; i < COSTUME_COUNT; ++i)
                {
                    uintptr_t addr = GetSheathAddr(i);
                    if (m_originalSheaths[i] != 0)
                        injector::WriteMemory<uint32_t>(addr, m_originalSheaths[i], true);
                }
            }
            m_isSamPatched = false;
        }
    }

    bool IsSamPatched() const { return m_isSamPatched; }

    uint32_t GetOriginalSheathId(uint32_t costumeIndex = 0) const
    {
        if (costumeIndex < COSTUME_COUNT && m_originalSheaths[costumeIndex] != 0)
            return m_originalSheaths[costumeIndex];
        return 0x11014u;
    }
};
