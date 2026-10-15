#pragma once
#include "SaiMovesetPolicy.h"
#include "SamPlayerIdentity.h"
#include "gui.h"
#include <PlayerManagerImplement.h>

class SaiMovesetController {
    Pl0000* owner = nullptr;
    SaiMovesetPolicy::Buffer buffer;
public:
    static SaiMovesetController& Get() { static SaiMovesetController value; return value; }
    void Forget(Pl0000* player) { if (owner == player) { owner = nullptr; buffer.Reset(); } }
    void ApplyAttackInput(Pl0000* player) {
        if (owner != player) { owner = player; buffer.Reset(); }
        DWORD foreground = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (!SamPlayerIdentity::Eligible(player) || !player->isAlive() ||
            !g_pPlayerManager || g_pPlayerManager->getCustomWeaponEquipped() != 3 ||
            !player->m_CustomWeaponHandle.getEntity() || gui::IsMenuVisible() ||
            foreground != GetCurrentProcessId() || g_StaFlags.STA_PAUSE ||
            g_StaFlags.STA_QTE || g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC ||
            g_StaFlags.STA_SOFT_EVENT || player->isBladeModeActive()) { buffer.Reset(); return; }
        if (Trigger::StpFlags.STP_OBJ) return;
        const bool heavy = (player->m_CurrentInput.m_Trig & player->m_ButtonHeavyAttack) != 0;
        const bool light = (player->m_CurrentInput.m_Trig & player->m_ButtonLightAttack) != 0;
        const unsigned action = buffer.Poll(player->m_Rno0, player->isInAir() != FALSE,
            heavy, light, player->m_Rno1 != 0, float(player->m_AnimationFrame));
        if (action) {
            player->m_CurrentInput.m_Trig &= ~player->m_ButtonHeavyAttack;
            player->setRno(action, 0, 0, 0);
        }
    }
};
