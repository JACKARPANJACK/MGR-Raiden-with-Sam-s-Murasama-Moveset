#pragma once

#include <Pl0000.h>

constexpr unsigned int CHARGE_TIER1_FRAMES = 15; // 0.25s
constexpr unsigned int CHARGE_TIER2_FRAMES = 45; // 0.75s
constexpr unsigned int CHARGE_TIER3_FRAMES = 75; // 1.25s (Max Judgement Cut)

class ChargeController
{
private:
    bool m_bChargeActive = false;
    bool m_bChargeRunActive = false;
    bool m_bAirChargeActive = false;
    unsigned int m_chargeFrames = 0;
    unsigned int m_chargeTier = 0;

public:
    ChargeController() = default;

    void Reset()
    {
        m_bChargeActive = false;
        m_bChargeRunActive = false;
        m_bAirChargeActive = false;
        m_chargeFrames = 0;
        m_chargeTier = 0;
    }

    bool IsActive() const { return m_bChargeActive || m_bChargeRunActive || m_bAirChargeActive; }
    bool IsChargeRun() const { return m_bChargeRunActive; }
    bool IsStanceCharge() const { return m_bChargeActive; }
    bool IsAirCharge() const { return m_bAirChargeActive; }
    unsigned int GetFrames() const { return m_chargeFrames; }
    unsigned int GetTier() const { return m_chargeTier; }

    void StartStanceCharge()
    {
        m_bChargeActive = true;
        m_bChargeRunActive = false;
        m_bAirChargeActive = false;
        m_chargeFrames = 0;
        m_chargeTier = 0;
    }

    void StartChargeRun()
    {
        m_bChargeRunActive = true;
        m_bChargeActive = false;
        m_bAirChargeActive = false;
        m_chargeFrames = 0;
        m_chargeTier = 0;
    }

    void StartAirCharge()
    {
        m_bAirChargeActive = true;
        m_bChargeActive = false;
        m_bChargeRunActive = false;
        m_chargeFrames = 0;
        m_chargeTier = 0;
    }

    void StopCharge()
    {
        m_bChargeActive = false;
        m_bChargeRunActive = false;
        m_bAirChargeActive = false;
        m_chargeFrames = 0;
        m_chargeTier = 0;
    }

    void IncrementFrames() { ++m_chargeFrames; }
    void SetTier(unsigned int tier) { m_chargeTier = tier; }

    void Tick(bool held, bool airborne)
    {
        if (!held)
        {
            StopCharge();
            return;
        }
        if (!IsActive())
            airborne ? StartAirCharge() : StartStanceCharge();
        IncrementFrames();
        if (m_chargeFrames >= CHARGE_TIER3_FRAMES) SetTier(3);
        else if (m_chargeFrames >= CHARGE_TIER2_FRAMES) SetTier(2);
        else if (m_chargeFrames >= CHARGE_TIER1_FRAMES) SetTier(1);
    }

    static bool IsChargeAction(unsigned int action)
    {
        return action == 0x100010 || action == 0x100011 || action == 0x10001A;
    }
};
