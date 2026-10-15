#pragma once
#include "SaiPullPolicy.h"
#include "WeaponSwitcher.h"
#include "SamTargets.h"
#include "SaiPullDiagnostics.h"
#include "SaiMovesetPolicy.h"
#include <CharacterControl.h>

class SaiPullController {
    Pl0000* owner = nullptr;
    EntityHandle victim{};
    float dragX=0,dragZ=0;
    unsigned ticks = 0, previousPad = 0;
    bool previousKey = false, strikePending = false;
    bool queued=false;
    bool Allowed(Pl0000* player, bool pulling = false) const {
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        return player && player->m_pEntity && g_pPlayerManager &&
            g_pPlayerManager->getCustomWeaponEquipped()==3 &&
            !gui::IsMenuVisible() && !g_StaFlags.STA_PAUSE && foreground==GetCurrentProcessId() &&
            SaiPullPolicy::Safe(player->m_Rno0, player->isAlive() != FALSE,
                player->isInAir() != FALSE, player->isBladeModeActive() != FALSE,
                g_StaFlags.STA_QTE || g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT, pulling);
    }
    void Release() {
        victim = nullptr; ticks = 0; strikePending = false;queued=false;
    }
public:
    void QueuePull(){queued=true;}
    static SaiPullController& Get() { static SaiPullController instance; return instance; }
    void Forget(Pl0000* player) { if (owner == player) { Release(); owner=nullptr; } }
    void ApplyAttackInput(Pl0000* player) {
        // Native pad input has been collected by the time HandleActions runs.
        const bool requested=queued;queued=false;
        PollInput(player,requested);
        if (strikePending && player == owner && Allowed(player,true)) {
            // HandleActions runs after native input/state selection. A synthetic
            // Heavy edge here cannot select an attack; enter its native action.
            if(!SaiPullPolicy::SaiAttack(player->m_Rno0))
                player->setRno(SaiMovesetPolicy::StabbingDespair,0,0,0);
            strikePending = false;
        }
    }
    void Tick(Pl0000* player) {
        if (owner != player) { Release(); owner = player; }
        if (!Allowed(player, ticks != 0)) {
            if(ticks)SaiPullDiagnostics::Report("Pull interrupted",player?player->m_Rno0:0);
            Release(); return;
        }
        if (Trigger::StpFlags.STP_OBJ) return;
    }
private:
    void PollInput(Pl0000* player, bool requested=false) {
        static_assert(SaiPullPolicy::LeftStickButton == Hw::PAD_BTN_L3);
        const bool key = (GetAsyncKeyState('F') & 0x8000) != 0;
        const unsigned pad = g_dbPad.m_On | g_dbPad.m_Trig;
        const bool press = requested || SaiPullPolicy::Press(key, previousKey, g_dbPad.m_On, g_dbPad.m_Trig, previousPad);
        previousKey = key; previousPad = pad;
        if (!Allowed(player, true)) {
            if(press) {
                const char* reason=!g_pPlayerManager || g_pPlayerManager->getCustomWeaponEquipped()!=3 ?
                    "Equip Sai to pull" : player && player->isInAir() ? "Pull requires ground contact" :
                    "Pull unavailable in this player state";
                SaiPullDiagnostics::Report(reason,player?player->m_Rno0:0);
            }
            Release(); return;
        }
        if (Trigger::StpFlags.STP_OBJ) return;
        DWORD foreground = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (!press || gui::IsMenuVisible() || foreground != GetCurrentProcessId()) return;
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
        if (!best) {
            auto* nearby=SamTargets::Find(player,SaiPullPolicy::Range);
            SaiPullDiagnostics::Report("No eligible cyborg in front within 15m",player->m_Rno0,
                nearby?unsigned(nearby->m_ObjId):0);return;
        }
        if(distance<=SaiPullPolicy::StopDistance) {SaiPullDiagnostics::Report("Target already within reach",player->m_Rno0,unsigned(best->m_ObjId),distance);return;}
        victim=best;
        const auto start=best->getTransPos();dragX=start.x;dragZ=start.z;
        ticks=SaiPullPolicy::Duration;
        SaiPullDiagnostics::Report("Pull started",player->m_Rno0,unsigned(best->m_ObjId),distance);
        strikePending = true;
    }
public:
    void PostTick(Pl0000* player) {
        Entity* target=victim.getEntity();
        if (!ticks) return;
        if (player != owner || !Allowed(player, true) || !target || !target->m_pBehavior || !target->isAlive()) {
            SaiPullDiagnostics::Report("Pull interrupted",player?player->m_Rno0:0);Release();return;
        }
        if (Trigger::StpFlags.STP_OBJ) return;
        auto p=target->getTransPos(); const auto origin=player->m_pEntity->getTransPos();
        // Advance the commanded drag path, not the AI's next attempted step.
        // This keeps a running enemy from undoing the physical pull each tick.
        p.x=dragX;p.z=dragZ;
        const float dx=origin.x-p.x, dz=origin.z-p.z, d=std::sqrt(dx*dx+dz*dz);
        const float step=SaiPullPolicy::Step(d);
        if (!step) {SaiPullDiagnostics::Report("Pull complete",player->m_Rno0,unsigned(target->m_ObjId),d);Release();return;}
        p.x+=dx/d*step; p.z+=dz/d*step;
        dragX=p.x;dragZ=p.z;
        // Cancel residual walking/knockback velocity before synchronizing the
        // actor, physics controller and entity after the enemy's native update.
        auto* actor=static_cast<BehaviorAppBase*>(target->m_pBehavior);
        actor->m_TransSpeed.x=0;actor->m_TransSpeed.z=0;
        target->m_pBehavior->setTransPos(p);
        if (target->m_pBehavior->m_pCharacterControl)
            target->m_pBehavior->m_pCharacterControl->setPosition(p,TRUE);
        target->setTransPos(p);
        if(d-step<=SaiPullPolicy::StopDistance+0.001f) {
            SaiPullDiagnostics::Report("Pull complete",player->m_Rno0,unsigned(target->m_ObjId),d-step);Release();
        }
        else if(--ticks==0) {SaiPullDiagnostics::Report("Pull travel ended",player->m_Rno0,unsigned(target->m_ObjId),d-step);Release();}
    }
};
