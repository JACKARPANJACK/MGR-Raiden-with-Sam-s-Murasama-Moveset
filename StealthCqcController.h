#pragma once
#include "StealthCqcPolicy.h"
#include "SamAttackInputPolicy.h"
#include "SamSheathPolicy.h"
#include "StealthCqcAnimations.h"
#include "StealthStanceController.h"
#include <BehaviorUtility.h>
#include <cSlowRateManager.h>

// MGSV attacker poses with MGR's damage, reaction and character collision.
// Victim reaction clips are native MGR; this is not a paired MGSV victim retarget.
class StealthCqcController {
    using Move=StealthCqcPolicy::Move;
    Pl0000* owner=nullptr;
    EntityHandle victim,previousExecutor;
    cSlowRate originalSlow;
    cSlowRateUnit* heldUnit=nullptr;
    Move move=Move::None,afterRise=Move::None;
    unsigned ticks=0,combo=0,direction=0;
    float frame=0;
    bool low=false,hit=false,queued=false,silent=false,priorF=false,priorB=false;
    SamAttackInputPolicy::Reader attacks;
    StealthCqcPolicy::Command pending=StealthCqcPolicy::Command::Native;
    unsigned commandDirection=0;
    StealthCqcPolicy::Buffer buffer;
    StealthCqcPolicy::Playback playback;
    bool inputSampled=false,takedownEdge=false;
    int nativeClip=-1;
    bool motionSupplied=false,sequenceSupplied=false;
    unsigned frameStartState=0;
    std::atomic<const char*> diagnostics{"Equip Unarmed for CQC"};
    const char* NativeCode(Pl0000* p,char (&code)[5]) const {
        code[0]=0;
        if(p && p->m_pAnimationSlot && p->m_pAnimationSlot->m_pArray && p->m_pAnimationSlot->m_Size>0)
            SamSheathPolicy::Code(code,p->m_pAnimationSlot->m_pArray[0].m_pAnimName);
        return code;
    }
    bool NativeAttack(Pl0000* p)const {char code[5];return p && (StealthCqcPolicy::NativeAttackAction(p->m_Rno0) || StealthCqcPolicy::NativeAttackClip(NativeCode(p,code)));}
    bool NativeRecovery(Pl0000* p)const {
        if(!p || !p->m_Rno1 || !NativeAttack(p))return false;
        if(StealthCqcPolicy::NativeLightFinisher(p->m_Rno0,p->m_Rno1,unsigned(p->field_2620)) &&
            p->m_AnimationFrame>=StealthCqcPolicy::NativeLightRecoveryFrame)return true;
        return StealthCqcPolicy::Recovery(true,p->getRemainingAnimationFrames(0),p->m_pSeqFlags[0]);
    }
    void Report(Pl0000* p,const char* text) {
        diagnostics=text;char code[5];
        GunRuntimeLog::Write("[CQC] %s state=%X/%u clip=%s equipped=%d swordState=%d selected=%d frame=%d move=%u combo=%u pair=%u/%u seq=%X nextPunch=%u",text,
            p?p->m_Rno0:0,p?p->m_Rno1:0,p?NativeCode(p,code):"",g_pPlayerManager?g_pPlayerManager->getCustomWeaponEquipped():-1,
            p?p->m_SwordState:-1,WeaponSwitcher::Get().Selected(),p?p->m_AnimationFrame:0,unsigned(move),combo,unsigned(motionSupplied),unsigned(sequenceSupplied),p?p->m_pSeqFlags[0]:0,p?unsigned(p->field_2620):0);
    }
    bool inputReserved=false;
    static void ReserveAttacks(Pl0000* p) {
        const unsigned mask=StealthStancePolicy::ButtonBits(p->m_ButtonLightAttack|p->m_ButtonHeavyAttack);
        p->m_CurrentInput.m_On&=~mask;p->m_CurrentInput.m_Trig&=~mask;p->m_CurrentInput.m_Rep&=~mask;
    }
    unsigned savedOn=0,savedTrig=0,savedRep=0,savedKeyOn=0,savedKeyTrig=0,savedKeyRep=0;
    void ReserveTakedown(Pl0000* p) {
        constexpr unsigned mask=StealthCqcPolicy::TakedownButton,key=1u<<('F'%32);
        savedOn=g_dbPad.m_On&mask;savedTrig=g_dbPad.m_Trig&mask;savedRep=g_dbPad.m_Rep&mask;
        savedKeyOn=g_Keyboard.m_pOn[2]&key;savedKeyTrig=g_Keyboard.m_pTrig[2]&key;savedKeyRep=g_Keyboard.m_pRep[2]&key;
        g_dbPad.m_On&=~mask;g_dbPad.m_Trig&=~mask;g_dbPad.m_Rep&=~mask;
        g_Keyboard.m_pOn[2]&=~key;g_Keyboard.m_pTrig[2]&=~key;g_Keyboard.m_pRep[2]&=~key;
        p->m_CurrentInput.m_On&=~mask;p->m_CurrentInput.m_Trig&=~mask;inputReserved=true;
    }
    int hp=0;
    bool Supported() const {return owner && g_pPlayerManager && StealthCqcPolicy::Unarmed(g_pPlayerManager->getCustomWeaponEquipped(),owner->m_SwordState,SamNativeRuntime::Get().Active(owner),g_GameStateManager.IsRoundTripActive);}
    bool Blocked() const {
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        return foreground!=GetCurrentProcessId() || gui::IsMenuVisible() || g_StaFlags.STA_PAUSE || Trigger::StpFlags.STP_OBJ;
    }
    bool Scripted() const {return g_StaFlags.STA_EVENT || g_StaFlags.STA_QTE || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT;}
    bool Clear(Pl0000* p,Entity* target) const {
        alignas(16) BehaviorUtility::RayCast ray;BehaviorUtility::RayInfo info{};
        info.m_TransStart=p->m_TransPos;info.m_TransEnd=target->getTransPos();
        info.m_TransStart.y+=.7f;info.m_TransEnd.y+=.7f;
        info.m_CollisionFilter=6;info.m_IgnoreObjectFilter=p->getCollisionFilter();
        info.m_bMultiHit=TRUE;
        if(!BehaviorUtility::checkRay(&ray,&info))return true;
        const auto& hits=ray.m_RayHitCollector.getHits();
        if(!hits.m_data || hits.m_size<0 || hits.m_size>4096)return false;
        for(int i=0;i<hits.m_size;++i) {
            const auto& h=hits.m_data[i];if(!std::isfinite(h.m_hitFraction) || h.m_hitFraction<0 || h.m_hitFraction>1)return false;
            if(h.m_hitFraction>=.9999f)continue;
            if(!h.field_50)return false;
            const auto data=reinterpret_cast<uintptr_t(__cdecl*)(const void*)>(shared::base+0x517CD0)(h.field_50);
            if(data==reinterpret_cast<uintptr_t>(target) || data==reinterpret_cast<uintptr_t>(target->m_pBehavior) ||
                data==reinterpret_cast<uintptr_t>(p) || data==reinterpret_cast<uintptr_t>(p->m_pEntity))continue;
            return false;
        }
        return true;
    }
    Entity* Find(Pl0000* p,bool rear) const {
        Entity* best=nullptr;float nearest=3;
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        const unsigned count=unsigned((std::max)(0,(std::min)(8192,list.getSize())));
        for(unsigned n=0;n<count && it!=Hw::cFixedList<Entity*>::iterator{};++n,++it) {
            Entity* e=*it;if(!e || !e->m_pBehavior || !e->isAlive() || !StealthCqcPolicy::Eligible(unsigned(e->m_ObjId)))continue;
            auto* enemy=static_cast<BehaviorEmBase*>(e->m_pBehavior);
            const auto d=e->getTransPos()-p->m_TransPos;
            const Entity* executor=enemy->m_QTETargetExecutor.getEntity();
            if(!StealthCqcPolicy::Reach(d.x,d.y,d.z,p->m_Rot.y) || (executor && executor!=p->m_pEntity) || !Clear(p,e))continue;
            if(rear && !StealthCqcPolicy::Rear(d.x,d.z,enemy->m_Rot.y,enemy->m_CautionStateManager.m_nState))continue;
            const float distance=d.x*d.x+d.z*d.z;if(distance<nearest){nearest=distance;best=e;}
        }
        return best;
    }
    void Unhold() {
        Entity* e=victim.getEntity();
        if(e && heldUnit && e->getSlowRate()->m_pUnit==heldUnit)*e->getSlowRate()=originalSlow;
        originalSlow.cleanup();heldUnit=nullptr;
        if(e && e->m_pBehavior && owner && e->m_pBehavior) {
            auto* enemy=static_cast<BehaviorAppBase*>(e->m_pBehavior);
            if(enemy->m_QTETargetExecutor.getEntity()==owner->m_pEntity)enemy->m_QTETargetExecutor=previousExecutor;
        }
    }
    bool Hold(Entity* target) {
        auto* slow=target->getSlowRate();if(!slow || !slow->m_pUnit || slow->m_pUnit->m_Rate<.5f)return false;
        originalSlow=*slow;slow->cleanup();
        if(!slow->setSlowType(eRateType_Em)){*slow=originalSlow;originalSlow.cleanup();return false;}
        heldUnit=slow->m_pUnit;heldUnit->m_Rate=.08f;
        auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);
        previousExecutor=enemy->m_QTETargetExecutor;enemy->m_QTETargetExecutor=owner->m_pEntity;
        enemy->requestAnimationByName("0320",0,.05f,1,0,0,1);
        return true;
    }
    void End(Pl0000* p) {
        Unhold();move=Move::None;victim=nullptr;queued=false;buffer.Clear();
        if(p && p==owner && p->m_Rno0==StealthCqcPolicy::Action) {
            p->setRno(p->isInAir()?0xB:0,0,0,0);
            const auto stance=StealthStanceController::Get().SelectedStance();
            if(!p->isInAir() && stance!=StealthStancePolicy::Stance::Standing)
                StealthStanceController::Get().CombatPose(StealthStancePolicy::BaseClip(stance,false),0);
        }
    }
    void Select(Move next) {move=next;frame=0;ticks=0;hit=false;nativeClip=-1;}
    bool StartMotion(Pl0000* p) {
        const int index=StealthCqcPolicy::Clip(move,low,combo,direction);
        const char* code=StealthCqcAnimations::Get().Code(index);
        if(!code){Report(p,"CQC MOT/BXM pair unavailable");return false;}
        // Clear each native attack descriptor's prior-hit ownership once per
        // strike, then let its BXM AttackTrack own damage and collision.
        if(move==Move::Combo) {
            for(uint16_t number:{uint16_t(196),uint16_t(202)}) {
                auto* table=*reinterpret_cast<uintptr_t**>(p);
                auto* descriptor=reinterpret_cast<CollisionAttackData*(__thiscall*)(Pl0000*,const uint16_t*)>(table[0x130/4])(p,&number);
                if(descriptor) {
                    auto* vtable=*reinterpret_cast<uintptr_t**>(descriptor);
                    reinterpret_cast<void(__thiscall*)(CollisionAttackData*,unsigned)>(vtable[1])(descriptor,1);
                }
            }
        }
        nativeClip=index;motionSupplied=sequenceSupplied=false;
        playback.Start(StealthStanceController::Get().ClipFrames(index));
        const int result=p->requestAnimationByName(code,0,.10f,1,0x8000000,0,1);
        if(result==-1 || !motionSupplied || !sequenceSupplied) {
            Report(p,"CQC native motion/sequence attachment failed");nativeClip=-1;return false;
        }
        Report(p,"CQC MOT and BXM attached");return true;
    }
    void Begin(Pl0000* p,Entity* target,Move next,bool stealth) {
        owner=p;victim=next==Move::Combo?nullptr:target;hp=p->m_Hp;silent=stealth;low=StealthStanceController::Get().SelectedStance()!=StealthStancePolicy::Stance::Standing;
        if(!StealthStanceController::Get().ClipFrames(StealthCqcPolicy::Clip(next,low)) ||
            !StealthCqcAnimations::Get().Clip(StealthCqcPolicy::Clip(next,low)).motion) {victim=nullptr;Report(p,"CQC animation bundle unavailable");return;}
        if(next!=Move::Combo && (!target || !Hold(target))) {victim=nullptr;Report(p,"Target unavailable for grab");return;}
        auto* states=reinterpret_cast<void**>(reinterpret_cast<char*>(p)+0x7CC);
        if(states[0]&&states[1])reinterpret_cast<void(__thiscall*)(void*,void*)>(shared::base+0x982DE0)(states[0],states[1]);
        p->setRno(StealthCqcPolicy::Action,0,0,0);combo=0;queued=false;afterRise=next;
        Select(StealthStanceController::Get().SelectedStance()==StealthStancePolicy::Stance::Prone?Move::Rise:next);
        Report(p,stealth?"Stealth takedown started":next==Move::Grab?"Grab started":"MGSV CQC combo started");
        if(!stealth)StealthStanceController::Get().Reveal();
    }
    void Impact(Pl0000* p) {
        if(move!=Move::Throw && move!=Move::Knife)return;
        Entity* e=victim.getEntity();if(!e || !e->m_pBehavior || !e->isAlive())return;
        const auto d=e->getTransPos()-p->m_TransPos;
        if(!StealthCqcPolicy::Reach(d.x,d.y,d.z,p->m_Rot.y,1.9f) || !Clear(p,e))return;
        auto* enemy=static_cast<BehaviorAppBase*>(e->m_pBehavior);
        const int amount=move==Move::Knife?enemy->m_Hp:(std::max)(1,enemy->m_HpMax/4);
        enemy->damage(amount,false);
        if(e->isAlive())enemy->requestAnimationByName(move==Move::Throw?"0340":"0320",0,.05f,1,0,0,1);
        if(move==Move::Throw && e->isAlive()) {
            Unhold();const float yaw=p->m_Rot.y+(direction==1?D3DX_PI:direction==2?-D3DX_PI*.5f:direction==3?D3DX_PI*.5f:0);
            Hw::cVec4 velocity(std::sin(yaw)*.16f,.10f,std::cos(yaw)*.16f,0);
            enemy->m_TransSpeed=velocity;
            if(enemy->m_pCharacterControl && enemy->m_pCharacterControl->m_pVelocity)*enemy->m_pCharacterControl->m_pVelocity=velocity;
        }
    }
public:
    static StealthCqcController& Get(){static StealthCqcController c;return c;}
    void RestoreInputs() {
        if(!inputReserved)return;
        constexpr unsigned mask=StealthCqcPolicy::TakedownButton,key=1u<<('F'%32);
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|savedOn;g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|savedTrig;g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|savedRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~key)|savedKeyOn;
        g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~key)|savedKeyTrig;g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~key)|savedKeyRep;
        inputReserved=false;
    }
    void Forget(Pl0000* p){if(owner==p){RestoreInputs();End(p);owner=nullptr;pending=StealthCqcPolicy::Command::Native;attacks.Reset();}}
    const char* Status()const{return diagnostics.load();}
    bool Active(Behavior* p)const{return p==owner && owner && move!=Move::None && owner->m_Rno0==StealthCqcPolicy::Action;}
    SamArchiveLookup::Clip RequestedClip(Behavior* p,const char* code) {
        const char* expected=nativeClip<0?nullptr:StealthCqcAnimations::Get().Code(nativeClip);
        return Active(p) && code && expected && !std::strcmp(code,expected)?StealthCqcAnimations::Get().Clip(nativeClip):SamArchiveLookup::Clip{};
    }
    void MotionSupplied(){motionSupplied=true;}
    void SequenceSupplied(){sequenceSupplied=true;}
    void BeforeTick(Pl0000* p) {
        RestoreInputs();pending=StealthCqcPolicy::Command::Native;inputSampled=false;takedownEdge=false;
        if(owner!=p){End(owner);owner=p;priorF=priorB=false;attacks.Reset();}
        frameStartState=p?p->m_Rno0:0;
        const bool f=(GetAsyncKeyState('F')&0x8000)!=0;
        const bool button=((g_dbPad.m_On|g_dbPad.m_Trig)&StealthCqcPolicy::TakedownButton)!=0;
        takedownEdge=(f&&!priorF)||(button&&!priorB);priorF=f;priorB=button;
        GunRuntimeLog::cqcSelected=Supported();
        if(!p)return;
        const bool safe=Supported() && !Scripted() && p->isAlive() && !p->isInAir() && !p->isBladeModeActive();
        if(move!=Move::None && (!safe || p->m_Hp<hp || p->m_Rno0!=StealthCqcPolicy::Action))End(p);
        if(Blocked())return;
        const bool wasBuffered=buffer.Active();buffer.Tick(safe);
        if(wasBuffered && !buffer.Active())Report(p,safe?"CQC buffer expired before native recovery":"CQC buffer cancelled by player context");
        if(safe)ReserveTakedown(p);
    }
    // Called from native virtual input dispatch after collection, before its
    // action-selection switch. Never sample attack edges at TickGame entry.
    void SampleNativeInput(Pl0000* p) {
        if(p!=owner || inputSampled)return;inputSampled=true;
        const auto input=attacks.Poll(p->m_CurrentInput.m_On,p->m_CurrentInput.m_Trig,p->m_ButtonLightAttack,p->m_ButtonHeavyAttack);
        const bool takedown=takedownEdge || ((p->m_CurrentInput.m_Trig&StealthCqcPolicy::TakedownButton)!=0);
        if(Blocked() || !Supported() || Scripted() || !p->isAlive() || p->isInAir() || p->isBladeModeActive()) {
            if((input.light||input.heavy||takedown) && g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()==5)Report(p,"CQC blocked by player/context state");
            return;
        }
        p->m_CurrentInput.m_On&=~StealthCqcPolicy::TakedownButton;p->m_CurrentInput.m_Trig&=~StealthCqcPolicy::TakedownButton;
        // Read the fresh edge above, then keep held/queued custom attacks out
        // of the native selector until their motion or recovery buffer ends.
        if(move!=Move::None || buffer.Active())ReserveAttacks(p);
        float x=g_dbPad.m_LeftStick.x,y=g_dbPad.m_LeftStick.y;
        const bool a=(GetAsyncKeyState('A')&0x8000)!=0,d=(GetAsyncKeyState('D')&0x8000)!=0;
        const bool w=(GetAsyncKeyState('W')&0x8000)!=0,s=(GetAsyncKeyState('S')&0x8000)!=0;
        if(a||d||w||s){x=float(d)-float(a);y=float(w)-float(s);}
        const auto dir=SamDirectionalPolicy::Resolve(x,y);
        auto command=StealthCqcPolicy::Resolve(dir,input.light,input.heavy,move==Move::Hold,takedown);
        const bool nativeAttack=NativeAttack(p);
        // A fresh neutral Light during a native punch buffers the MGSV extension.
        if(move==Move::None)command=StealthCqcPolicy::ExtendNativePunch(command,
            StealthCqcPolicy::NativeLightFinisher(p->m_Rno0,p->m_Rno1,unsigned(p->field_2620)),dir,input.light,input.heavy);
        commandDirection=StealthCqcPolicy::ThrowDirection(dir);
        // Consume a native recovery window BEFORE its input dispatcher can
        // select the next AF/B0 action and replace the flags/frame we observed.
        if(move==Move::None && buffer.Active() && NativeRecovery(p)) {
            commandDirection=buffer.direction;pending=buffer.Take(true);ReserveAttacks(p);
            Report(p,"Buffered command captured at native sequence recovery");return;
        }
        if(move==Move::Combo) {
            if(input.light && !input.heavy && combo<3){queued=true;Report(p,"Next MGSV CQC hit buffered");}
            return;
        }
        if(move!=Move::None && move!=Move::Hold)return;
        const bool aiming=(GetAsyncKeyState('C')&0x8000)||((p->m_CurrentInput.m_On|g_dbPad.m_On)&StealthStancePolicy::ButtonBits(p->m_ButtonUseSubweapon));
        if(command!=StealthCqcPolicy::Command::Takedown && aiming){buffer.Clear();return;}
        if(command==StealthCqcPolicy::Command::Native && input.light && !input.heavy && !nativeAttack)
            Report(p,"Native unarmed punch; finish Light chain to append MGSV CQC");
        const bool neutral=(StealthStancePolicy::Locomotion(frameStartState)||StealthStancePolicy::Locomotion(p->m_Rno0)) && !nativeAttack;
        if(command!=StealthCqcPolicy::Command::Native && (neutral || nativeAttack || move==Move::Hold)) {
            if(nativeAttack && move==Move::None && !NativeRecovery(p)) {buffer.Set(command,commandDirection);Report(p,"CQC extension buffered for native punch recovery");}
            else pending=command;
            // Reserve only accepted custom commands; neutral native punches and
            // simultaneous attack/dodge chords keep their native dispatch.
            const unsigned mask=StealthStancePolicy::ButtonBits((input.light?p->m_ButtonLightAttack:0)|(input.heavy?p->m_ButtonHeavyAttack:0));
            p->m_CurrentInput.m_On&=~mask;p->m_CurrentInput.m_Trig&=~mask;p->m_CurrentInput.m_Rep&=~mask;
        }
    }
    void AfterNativeInput(Pl0000* p) {
        if(p!=owner || move==Move::None || p->m_Rno0==StealthCqcPolicy::Action || !Supported() ||
            Blocked() || Scripted() || !p->isAlive() || p->isInAir() || p->isBladeModeActive() || p->m_Hp!=hp)return;
        // Some native unarmed input branches select a basic punch/idle from
        // held input. Retain only our owned action, never overwrite damage,
        // falling, executions or script states.
        if(p->m_Rno0<=3 || StealthCqcPolicy::NativeAttackAction(p->m_Rno0)) {
            p->setRno(StealthCqcPolicy::Action,0,0,0);Report(p,"Retained owned CQC action after native input");
        }
    }
    bool HandleAction(Pl0000* p) {
        if(p!=owner)return false;
        SampleNativeInput(p); // Fallback if a native state bypassed virtual input dispatch.
        using Command=StealthCqcPolicy::Command;
        auto command=pending;pending=Command::Native;
        if(command==Command::Native && buffer.Active()) {
            const bool nativeAttack=NativeAttack(p);
            if(!Supported() || Blocked() || Scripted() || !p->isAlive() || p->isInAir() || p->isBladeModeActive() ||
                (!StealthStancePolicy::Locomotion(p->m_Rno0) && !nativeAttack))buffer.Clear();
            const bool ready=(StealthStancePolicy::Locomotion(p->m_Rno0) && !nativeAttack) ||
                (nativeAttack && NativeRecovery(p));
            const unsigned bufferedDirection=buffer.direction;command=buffer.Take(ready);
            if(command!=Command::Native)commandDirection=bufferedDirection;
        }
        if(!Blocked() && Supported() && !Scripted() && p->isAlive() && !p->isInAir() && !p->isBladeModeActive()) {
            if(move==Move::Hold) {
                if(command==Command::Release){End(p);return true;}
                if(command==Command::Throw){direction=commandDirection;Select(Move::Throw);StealthStanceController::Get().Reveal();}
                else if(command==Command::Knife){Select(Move::Knife);StealthStanceController::Get().Reveal();}
                else if(command==Command::Combo){Unhold();combo=0;Select(Move::Combo);StealthStanceController::Get().Reveal();}
            } else if(move==Move::None) {
                if(command==Command::Takedown){if(Entity* e=Find(p,true))Begin(p,e,Move::Knife,true);else Report(p,"No unaware cyborg behind within CQC reach");}
                else if(command==Command::Grab){if(Entity* e=Find(p,false))Begin(p,e,Move::Grab,false);else Report(p,"No reachable cyborg for grab");}
                else if(command==Command::Combo)Begin(p,Find(p,false),Move::Combo,false);
            }
        }
        if(move==Move::None)return false;
        if(p->m_Rno0!=StealthCqcPolicy::Action){End(p);return false;}
        if(Blocked())return true;
        Entity* e=victim.getEntity();
        if(Scripted() || !Supported() || !p->isAlive() || p->isInAir() || p->isBladeModeActive() || p->m_Hp<hp ||
            (move!=Move::Combo && !(move==Move::Rise && afterRise==Move::Combo) && (!e || (!e->isAlive() && !(hit && move==Move::Knife))))){End(p);return false;}
        if(e && heldUnit && static_cast<BehaviorAppBase*>(e->m_pBehavior)->m_QTETargetExecutor.getEntity()!=p->m_pEntity){End(p);return false;}
        if(silent)StealthStanceController::Get().QuietCqcProfile();
        if(e && heldUnit) {const auto d=e->getTransPos()-p->m_TransPos;if(std::hypot(d.x,d.z)>2.0f || std::fabs(d.y)>.8f){End(p);return false;}}
        if(nativeClip<0 && !StartMotion(p)){End(p);return false;}
        p->m_TransSpeed={0,-.015f,0,0};p->field_1020=p->m_TransSpeed;
        if(p->m_pCharacterControl && p->m_pCharacterControl->m_pVelocity)*p->m_pCharacterControl->m_pVelocity=p->m_TransSpeed;
        reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(p,1,1);
        const int clip=StealthCqcPolicy::Clip(move,low,combo,direction);
        const unsigned length=StealthStanceController::Get().ClipFrames(clip);if(length<2){End(p);return true;}
        ++ticks;
        if(ticks>600){Report(p,"CQC motion timeout");End(p);return true;}
        if(!playback.Observe(float(p->m_AnimationFrame)))return true;
        frame=playback.frame;
        // Punch damage comes exclusively from the native BXM collision track.
        // Held throws/knife retain one targeted event; no duplicate punch damage.
        if(!hit && frame>=StealthCqcAnimations::Get().Contact(clip) && (move==Move::Throw || move==Move::Knife)){hit=true;Impact(p);}
        if(move==Move::Hold) {
            if(ticks>300){End(p);return true;}
            if(playback.Finished()){nativeClip=-1;if(!StartMotion(p)){End(p);return false;}}
        }
        else if(playback.Finished() || (move==Move::Combo && StealthCqcPolicy::ComboRecovered(frame,StealthCqcAnimations::Get().Recovery(clip)))) {
            if(move==Move::Rise){Select(afterRise);Report(p,"Prone rise completed; CQC follows");}
            else if(move==Move::Grab){Select(Move::Hold);Report(p,"Grab completed; hold controls ready");}
            else if(move==Move::Combo && StealthCqcPolicy::ChainReady(queued,combo,frame,StealthCqcAnimations::Get().Recovery(clip))){++combo;queued=false;Select(Move::Combo);Report(p,"MGSV CQC combo advanced");}
            else {Report(p,"CQC motion completed; native controls released");End(p);return true;}
        }
        // MOT/BXM runs on the native animation graph, including interpolation,
        // hierarchical bones, collision windows, flags, sound and particles.
        return true;
    }
};
