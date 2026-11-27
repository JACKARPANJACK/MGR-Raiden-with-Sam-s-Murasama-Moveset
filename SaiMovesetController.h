#pragma once
#include "SaiMovesetPolicy.h"
#include "SamAttackInputPolicy.h"
#include "SamPlayerIdentity.h"
#include "GunRuntimeLog.h"
#include "gui.h"
#include <PlayerManagerImplement.h>
#include <cstddef>

static_assert(offsetof(Pl0000,m_TransSpeed)==0x890 && offsetof(Pl0000,field_948)==0x948 &&
    offsetof(Pl0000,field_2654)==0x2654,"Sai aerial physics ABI mismatch");

class SaiMovesetController {
    Pl0000* owner=nullptr;
    SaiMovesetPolicy::Buffer buffer;
    SaiMovesetPolicy::Machine machine;
    SaiMovesetPolicy::HeavyChain chain;
    SaiMovesetPolicy::AirChain airChain;
    SaiMovesetPolicy::AerialLift lift;
    SamAttackInputPolicy::Reader input;
    bool sampled=false,dispatching=false;
    unsigned pending=0,mask=0,on=0,trig=0,rep=0,padOn=0,padTrig=0,padRep=0;
    static float AirVelocity(Pl0000* p){return p->m_TransSpeed.y;}
    static void SetAirVelocity(Pl0000* p,float value){p->m_TransSpeed.y=value;}
    void EndAirMove(Pl0000* p) {
        // Native 9E clears its air-attack flag in landing phase 4. A cancel
        // or timeout must release it too; retain normal falling velocity.
        p->field_2654&=~4;
        p->field_948=0;
        if(AirVelocity(p)>0)SetAirVelocity(p,0);
        lift.Reset();
    }
    bool Allowed(Pl0000* player) const {
        return SamPlayerIdentity::Eligible(player) && player->isAlive() &&
            g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()==3 &&
            player->m_CustomWeaponHandle.getEntity() && !player->isBladeModeActive() &&
            !g_StaFlags.STA_QTE && !g_StaFlags.STA_EVENT && !g_StaFlags.STA_CODEC && !g_StaFlags.STA_SOFT_EVENT;
    }
    bool Paused() const {
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        return gui::IsMenuVisible() || foreground!=GetCurrentProcessId() || g_StaFlags.STA_PAUSE;
    }
    void ReserveHeavy(Pl0000* p) {
        mask=p->m_ButtonHeavyAttack&~0xFu;
        on=p->m_CurrentInput.m_On&mask;trig=p->m_CurrentInput.m_Trig&mask;rep=p->m_CurrentInput.m_Rep&mask;
        padOn=g_dbPad.m_On&mask;padTrig=g_dbPad.m_Trig&mask;padRep=g_dbPad.m_Rep&mask;
        p->m_CurrentInput.m_On&=~mask;p->m_CurrentInput.m_Trig&=~mask;p->m_CurrentInput.m_Rep&=~mask;
        g_dbPad.m_On&=~mask;g_dbPad.m_Trig&=~mask;g_dbPad.m_Rep&=~mask;
    }
public:
    static SaiMovesetController& Get(){static SaiMovesetController value;return value;}
    void Reset(){buffer.Reset();chain.Reset();airChain.Reset();pending=0;}
    void Forget(Pl0000* p){if(owner==p){AfterNativeInput(p);Reset();lift.Reset();machine.Reset();input.Reset();owner=nullptr;}}
    void BeforeTick(Pl0000* p) {
        if(owner!=p){Reset();lift.Reset();machine.Reset();input.Reset();owner=p;}
        sampled=false;pending=0;
        if(!p || !SaiMovesetPolicy::Find(p->m_Rno0)) {
            if(p && lift.active)EndAirMove(p);
            lift.Reset();machine.Reset();
        }
    }
    bool OwnsNativeInput(Pl0000* p) const {
        return p==owner && SaiMovesetPolicy::Find(p->m_Rno0)!=nullptr;
    }
    float TransitionFrame(Pl0000* p) const {
        if(p==owner && machine.action==p->m_Rno0) {
            if(machine.Recovering(p->m_Rno1))
                return (std::max)(float(machine.ticks),SaiMovesetPolicy::CancelFrame(machine.action));
            return float(machine.ticks);
        }
        return float(p->m_AnimationFrame);
    }
    void SampleNativeInput(Pl0000* p) {
        if(owner!=p){Reset();lift.Reset();machine.Reset();input.Reset();owner=p;}
        if(sampled)return;sampled=true;
        const auto attack=input.Poll(p->m_CurrentInput.m_On|g_dbPad.m_On,
            p->m_CurrentInput.m_Trig|g_dbPad.m_Trig,p->m_ButtonLightAttack,p->m_ButtonHeavyAttack);
        if(!Allowed(p) || Paused()){Reset();return;}
        const unsigned state=p->m_Rno0;
        const float frame=TransitionFrame(p);
        const bool initialized=p->m_Rno1!=0;
        const bool jump=(p->m_CurrentInput.m_Trig&(p->m_ButtonJump&~0xFu))!=0;
        if(!Trigger::StpFlags.STP_OBJ && OwnsNativeInput(p) && initialized && frame>=SaiMovesetPolicy::CancelFrame(state) && (jump || attack.light)) {
            if(state==SaiMovesetPolicy::UnavoidableFate)EndAirMove(p);
            // Replay the untouched Jump/Light edge through native selection.
            // Forcing jump state 9 here bypasses native takeoff initialization
            // and can leave a grounded player waiting in that state forever.
            Reset();machine.Reset();p->setRno(p->isInAir()?0xB:0,0,0,0);return;
        }
        const bool airborne=p->isInAir()!=FALSE;
        const bool light=attack.light||attack.lightHeld;
        const bool airReady=airChain.Observe(state,airborne,initialized,light);
        const bool airContext=airborne && SaiMovesetPolicy::AirChain::Context(state);
        if(airContext)chain.Reset();
        const bool groundReady=!airContext && chain.Poll(state,attack.heavy,light);
        // Ground: first two presses stay native. Air: the first actual native
        // air Heavy stays native, then a fresh press unlocks the second strike.
        const bool enabled=airContext ? airReady : groundReady;
        if(!enabled) {
            buffer.Reset();pending=0;return;
        }
        pending=buffer.Poll(state,p->isInAir()!=FALSE,attack.heavy,attack.light||attack.lightHeld,initialized,frame,Trigger::StpFlags.STP_OBJ);
        // Collect before the native selector, while 9C/9D and the fresh edge
        // still describe the source move. Prevent native selection stealing Y.
        if(!attack.light && !attack.lightHeld && (pending || buffer.ticks))ReserveHeavy(p);
    }
    void AfterNativeInput(Pl0000* p) {
        if(!mask)return;
        p->m_CurrentInput.m_On=(p->m_CurrentInput.m_On&~mask)|on;
        p->m_CurrentInput.m_Trig=(p->m_CurrentInput.m_Trig&~mask)|trig;
        p->m_CurrentInput.m_Rep=(p->m_CurrentInput.m_Rep&~mask)|rep;
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|padOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|padTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|padRep;mask=0;
    }
    void ApplyAttackInput(Pl0000* p) {
        if(!sampled){SampleNativeInput(p);AfterNativeInput(p);}
        if(!pending || !Allowed(p) || Paused() || Trigger::StpFlags.STP_OBJ)return;
        const unsigned next=pending;pending=0;
        if(!machine.Enter(next))return;
        if(next==SaiMovesetPolicy::UnavoidableFate) {
            airChain.Consume();lift.Start(p->m_TransPos.y);
            SetAirVelocity(p,lift.Velocity(AirVelocity(p),0,0,p->m_TransPos.y));
        }
        buffer.Reset();
        p->m_CurrentInput.m_Trig&=~(p->m_ButtonHeavyAttack&~0xFu);
        p->setRno(next,0,0,0);
        GunRuntimeLog::Write("[SaiMoveset] enter=%X menu=%u native=%X",next,SaiMovesetPolicy::Find(next)->menu,machine.native);
    }
    unsigned MapMotion(Behavior* actor,unsigned index) const {
        if(!dispatching || actor!=owner)return index;
        const unsigned mapped=SaiMovesetPolicy::Motion(machine.action,index);
        GunRuntimeLog::Write("[SaiMoveset] motion action=%X native=%X index=%u mapped=%u",machine.action,machine.native,index,mapped);
        return mapped;
    }
    void PostTick(Pl0000* p) {
        if(!OwnsNativeInput(p) || !Allowed(p))return;
        p->field_1428=1.0f;
        auto* weapon=p->m_CustomWeaponHandle.getEntity();
        if(!weapon || !weapon->m_pBehavior)return;
        auto* actor=weapon->m_pBehavior;actor->onDisp();
        for(int i=0;actor->m_pMeshes && i<actor->m_MeshNum;++i) {
            actor->m_pMeshes[i].m_MeshFlags|=1u;
            actor->m_pMeshes[i].m_Color.a=1;
            actor->m_pMeshes[i].m_ColorScale.a=1;
        }
    }
    bool HandleAction(Pl0000* p,void(__thiscall* nativeDispatch)(Pl0000*)) {
        if(!SaiMovesetPolicy::Find(p->m_Rno0))return false;
        if(!Allowed(p)) {
            if(lift.active)EndAirMove(p);
            Reset();p->setRno(p->isInAir()?0xB:0,0,0,0);return false;
        }
        if(Paused() || Trigger::StpFlags.STP_OBJ)return true;
        if(machine.action!=p->m_Rno0)machine.Enter(p->m_Rno0);
        const unsigned custom=machine.action;
        if(custom==SaiMovesetPolicy::UnavoidableFate) {
            // The first air Heavy leaves 9E's repeat-attack gravity suppression
            // set. Use its normal velocity integration for this added strike.
            p->field_948=0;
            if(SaiMovesetPolicy::AerialLift::Landed(machine.native,p->m_Rno1,p->isInAir()!=FALSE)) {
                p->m_Rno1=4; // 9E landing animation, followed by its normal exit
                GunRuntimeLog::Write("[SaiMoveset] aerial falling loop -> landing recovery");
            }
            if(lift.Supports(machine.ticks,p->m_Rno1,p->m_TransPos.y)) {
                SetAirVelocity(p,lift.Velocity(AirVelocity(p),machine.ticks,p->m_Rno1,p->m_TransPos.y));
            }
        }
        // Execute the native combat implementation, retaining its substate,
        // projectile callbacks, hit volumes, physics and recovery transitions.
        // Restore the registered identity before returning to the game.
        p->m_Rno0=machine.native;
        dispatching=true;
        __try {if(nativeDispatch)nativeDispatch(p);}
        __finally {dispatching=false;}
        const unsigned result=p->m_Rno0;
        p->m_Rno0=machine.Complete(result);
        // Native startup can reset the velocity. Reapply the one-time boost
        // after initialization so the second air strike keeps its clearance.
        if(custom==SaiMovesetPolicy::UnavoidableFate && machine.action && machine.ticks==1 &&
           lift.Supports(0,p->m_Rno1,p->m_TransPos.y)) {
            p->field_948=0;
            SetAirVelocity(p,lift.Velocity(AirVelocity(p),0,p->m_Rno1,p->m_TransPos.y));
        }
        if(machine.action && machine.Finished(p->m_Rno1,p->getRemainingAnimationFrames(0))) {
            p->setRno(p->isInAir()?0xB:0,0,0,0);
            machine.Reset();
        }
        if(p->m_Rno0!=custom) {
            if(custom==SaiMovesetPolicy::UnavoidableFate) {
                EndAirMove(p);
                if(p->m_Rno0==0xB && !p->isInAir())p->setRno(0,0,0,0);
                buffer.Reset();chain.Reset();pending=0; // spent until landing
            }
            else Reset();
            GunRuntimeLog::Write("[SaiMoveset] exit=%X to=%X",custom,p->m_Rno0);
        }
        return true;
    }
};
