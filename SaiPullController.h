#pragma once
#include "SaiPullPolicy.h"
#include "WeaponSwitcher.h"
#include "SamTargets.h"
#include <cSlowRateManager.h>
#include <CharacterControl.h>

class SaiPullController {
    Pl0000* owner = nullptr;
    EntityHandle victim{};
    cSlowRate savedSlow;
    cSlowRateUnit* localSlow = nullptr;
    unsigned ticks = 0, cooldown = 0, previousPad = 0, delay = 0;
    bool previousKey = false, strikePending = false;
    bool Allowed(Pl0000* player) const {
        return player && player->m_pEntity && WeaponSwitcher::Get().Selected() == 3 &&
            SaiPullPolicy::Safe(player->m_Rno0, player->isAlive() != FALSE,
                player->isInAir() != FALSE, player->isBladeModeActive() != FALSE,
                g_StaFlags.STA_QTE || g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT);
    }
    void Release() {
        Entity* target = victim.getEntity();
        if (target && localSlow && target->getSlowRate()->m_pUnit == localSlow)
            *target->getSlowRate() = savedSlow;
        savedSlow.cleanup(); localSlow = nullptr; victim = nullptr; ticks = delay = 0; strikePending = false;
    }
public:
    static SaiPullController& Get() { static SaiPullController instance; return instance; }
    void Forget(Pl0000* player) { if (owner == player) { Release(); owner=nullptr; cooldown=0; } }
    void ApplyAttackInput(Pl0000* player) {
        if (strikePending && player == owner && Allowed(player)) {
            player->m_CurrentInput.m_Trig |= player->m_ButtonHeavyAttack;
            strikePending = false;
        }
    }
    void Tick(Pl0000* player) {
        if (owner != player) { Release(); owner = player; cooldown = 0; }
        const bool key = (GetAsyncKeyState('F') & 0x8000) != 0;
        const unsigned pad = g_dbPad.m_On | g_dbPad.m_Trig;
        const bool press = (key && !previousKey) || ((pad & 0x1000) && !(previousPad & 0x1000) && !(pad & 0x8000));
        previousKey = key; previousPad = pad;
        if (Trigger::StpFlags.STP_OBJ) return;
        if (cooldown) --cooldown;
        if (!Allowed(player)) { Release(); return; }
        DWORD foreground = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (!press || cooldown || gui::IsMenuVisible() || foreground != GetCurrentProcessId()) return;
        // Choose a live whole cyborg in front of Raiden; heavy cyborgs are filtered
        // before selection so they cannot mask an eligible soldier behind them.
        Entity* best = nullptr; float distance = SaiPullPolicy::Range;
        auto& list = g_EntitySystem.getEntityList(); auto it = list.begin();
        const auto origin = player->m_pEntity->getTransPos();
        const unsigned count = unsigned((std::max)(0, (std::min)(8192, list.getSize())));
        for (unsigned n = 0; n < count && it != Hw::cFixedList<Entity*>::iterator{}; ++n, ++it) {
            Entity* candidate = *it;
            if (!candidate || !candidate->m_pBehavior || !candidate->isAlive() ||
                !SaiPullPolicy::Eligible(unsigned(candidate->m_ObjId))) continue;
            const auto p = candidate->getTransPos();
            const float dx=p.x-origin.x, dy=p.y-origin.y, dz=p.z-origin.z;
            const float d=std::sqrt(dx*dx+dy*dy+dz*dz);
            if (std::fabs(dy)>2.0f || dx*std::sin(player->m_Rot.y)+dz*std::cos(player->m_Rot.y)<0 || d>=distance) continue;
            best=candidate; distance=d;
        }
        if (!best || distance <= SaiPullPolicy::StopDistance) return;
        auto* slow = best->getSlowRate();
        if (!slow || !slow->m_pUnit || slow->m_pUnit->m_Rate < 0.5f) return;
        savedSlow = *slow; slow->cleanup();
        if (!slow->setSlowType(eRateType_Em)) { *slow=savedSlow; savedSlow.cleanup(); return; }
        victim=best; localSlow=slow->m_pUnit; localSlow->m_Rate=0.15f;
        ticks=SaiPullPolicy::Duration; cooldown=SaiPullPolicy::Cooldown; delay=12;
        // Let the equipped Sai's native heavy animation/hit handling run.
        strikePending = true;
    }
    void PostTick(Pl0000* player) {
        if (Trigger::StpFlags.STP_OBJ) return;
        Entity* target=victim.getEntity();
        if (!ticks) return;
        if (player != owner || !Allowed(player) || !target || !target->m_pBehavior || !target->isAlive()) { Release(); return; }
        if (delay) { --delay; return; }
        auto p=target->getTransPos(); const auto origin=player->m_pEntity->getTransPos();
        const float dx=origin.x-p.x, dz=origin.z-p.z, d=std::sqrt(dx*dx+dz*dz);
        const float step=SaiPullPolicy::Step(d);
        if (!step || --ticks == 0) { Release(); return; }
        p.x+=dx/d*step; p.z+=dz/d*step;
        target->m_pBehavior->setTransPos(p);
        if (target->m_pBehavior->m_pCharacterControl)
            target->m_pBehavior->m_pCharacterControl->setPosition(p,TRUE);
        target->setTransPos(p);
    }
};
