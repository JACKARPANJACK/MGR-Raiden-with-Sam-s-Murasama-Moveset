#pragma once
#include <mutex>
#include <cstdint>

class SamModelPatcher
{
private:
    mutable std::mutex m_mutex;
    bool m_isSamPatched = false;

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
        m_isSamPatched = isSam;
        // Sheath model lifecycle, dynamic hip attachment, and safe reversibility
        // are owned by SheathController. Avoid raw memory writes to instruction opcodes
        // which crash Raiden's engine on attack.
    }

    bool IsSamPatched() const { return m_isSamPatched; }
};
