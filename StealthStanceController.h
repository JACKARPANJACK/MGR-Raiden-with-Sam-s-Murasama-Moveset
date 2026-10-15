#pragma once
#include "StealthStancePolicy.h"
#include "StealthCqcPolicy.h"
#include "SamSheathPolicy.h"
#include "StealthPoseFile.h"
#include "StealthAdditivePose.h"
#include "StealthInputPolicy.h"
#include "StealthMotionPolicy.h"
#include "StealthDetectionPolicy.h"
#include "GunAimMath.h"
#include "SamPlayerIdentity.h"
#include "SamConfig.h"
#include <atomic>
#include <cstdio>

// Native animation/state and collision remain active. These matrices supply
// the retargeted stance after native update and are restored before its next tick.
class StealthStanceController {
    using Stance=StealthStancePolicy::Stance;
    struct Saved {short bone;cParts* part;Hw::cMtx matrix;unsigned short flags;};
    struct Sample {short bone;D3DXMATRIX from,last;};
    EntityHandle visualOwner;
    Pl0000* owner=nullptr;
    StealthStancePolicy::Cycle cycle;
    StealthPoseFile::Clip clips[34];
    std::vector<Saved> saved,releaseSaved;
    EntityHandle releaseOwner;
    bool nativeReleaseReady=false;
    std::vector<Sample> samples;
    std::atomic<const char*> status{"Standing"};
    int current=-1;
    unsigned blend=12;
    float phase=0;
    bool ready=false,attempted=false,positionValid=false,reserved=false;
    unsigned padOn=0,padTrig=0,padRep=0,reserveMask=0;
    bool jumpReserved=false;
    unsigned jumpMask=0,jumpKeyWord=0,jumpKeyMask=0,jumpKeyOn=0,jumpKeyTrig=0,jumpKeyRep=0;
    void ReservePad(unsigned mask) {
        const unsigned fresh=mask&~reserveMask;
        padOn|=g_dbPad.m_On&fresh;padTrig|=g_dbPad.m_Trig&fresh;padRep|=g_dbPad.m_Rep&fresh;
        reserveMask|=mask;g_dbPad.m_On&=~mask;g_dbPad.m_Trig&=~mask;g_dbPad.m_Rep&=~mask;reserved=true;
    }
    void BlockJump(Pl0000* p) {
        jumpMask=StealthStancePolicy::ButtonBits(p->m_ButtonJump);ReservePad(jumpMask);
        p->m_CurrentInput.m_On&=~jumpMask;p->m_CurrentInput.m_Trig&=~jumpMask;
        const unsigned key=unsigned((std::clamp)(SamConfig::GetInt("Stealth","JumpKey",VK_SPACE),0,255));
        jumpKeyWord=key/32;jumpKeyMask=1u<<(key%32);
        jumpKeyOn=g_Keyboard.m_pOn[jumpKeyWord]&jumpKeyMask;jumpKeyTrig=g_Keyboard.m_pTrig[jumpKeyWord]&jumpKeyMask;jumpKeyRep=g_Keyboard.m_pRep[jumpKeyWord]&jumpKeyMask;
        g_Keyboard.m_pOn[jumpKeyWord]&=~jumpKeyMask;g_Keyboard.m_pTrig[jumpKeyWord]&=~jumpKeyMask;g_Keyboard.m_pRep[jumpKeyWord]&=~jumpKeyMask;
        jumpReserved=true;
    }
    std::atomic<unsigned> detectionStance{0};
    unsigned revealTicks=0,motionTicks=0,motionStage=0;
    int lastHp=0;
    bool previousStanceKey=false,poseApplied=false;
    int combatClip=-1;float combatFrame=0;
    float aimWeight=0;int lastAimClip=12;float lastAimFrame=12;
    StealthInputPolicy::JumpLatch jumpLatch;
    void SampleJump(Pl0000* p,bool down) {
        if(!jumpLatch.Observe(down))return;
        int side=0;const unsigned input=g_dbPad.m_On|p->m_CurrentInput.m_On;
        if((input&Hw::PAD_BTN_LAL)||(GetAsyncKeyState('A')&0x8000))side=-1;
        if((input&Hw::PAD_BTN_LAR)||(GetAsyncKeyState('D')&0x8000))side=1;
        if(motion!=StealthMotionPolicy::Move::None && !MotionActive(p))EndMotion(p,false);
        const auto request=StealthMotionPolicy::Request(cycle.Selected(),true,side,
            motion==StealthMotionPolicy::Move::None && BaseGround(p) && !Paused() && Load());
        if(request!=StealthMotionPolicy::Move::None)BeginMotion(p,request);
    }
    static bool SampleMatrix(const StealthPoseFile::Clip& clip,short bone,float frame,D3DXMATRIX& out) {
        const auto track=std::find_if(clip.tracks.begin(),clip.tracks.end(),[&](const auto& t){return t.bone==bone;});
        if(track==clip.tracks.end())return false;
        frame=(std::clamp)(frame,0.0f,float(clip.frames-1));const unsigned lo=unsigned(frame),hi=(std::min)(lo+1,clip.frames-1);
        D3DXMATRIX a,b;std::memcpy(&a,clip.bytes.data()+track->offset+lo*64,64);std::memcpy(&b,clip.bytes.data()+track->offset+hi*64,64);
        return GunAimMath::BlendMatrix(a,b,frame-lo,out);
    }
    StealthMotionPolicy::Move motion=StealthMotionPolicy::Move::None;
    Hw::cVec4 motionDirection{};
    float motionPhase=0,motionYaw=0;
    Hw::cVec4 before{};
    bool Focused() const {DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);return process==GetCurrentProcessId();}
    bool Paused() const {return !Focused()||gui::IsMenuVisible()||g_StaFlags.STA_PAUSE||Trigger::StpFlags.STP_OBJ;}
    bool Scripted() const {return SamMovesetManager::StoryEvent()||g_StaFlags.STA_EVENT||g_StaFlags.STA_QTE||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT;}
    bool NativeWeaponAction(unsigned action) const {return StealthStancePolicy::GroundWeaponAction(action);}
    bool BaseGround(Pl0000* p) const {
        if(!p || !p->isAlive() || p->isInAir() || p->isBladeModeActive() || Scripted() ||
            Trigger::StaFlags.STA_NINJARUN || !(StealthStancePolicy::Locomotion(p->m_Rno0) || p->m_Rno0==0x10000E || (NativeWeaponAction(p->m_Rno0)&&WeaponPose(p))))return false;
        if(g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()==5 && p->m_pAnimationSlot &&
            p->m_pAnimationSlot->m_pArray && p->m_pAnimationSlot->m_Size>0) {
            char code[5]{};SamSheathPolicy::Code(code,p->m_pAnimationSlot->m_pArray[0].m_pAnimName);
            if(StealthCqcPolicy::NativeAttackClip(code))return false;
        }
        return true;
    }
    bool WeaponPose(Pl0000* p) const {
        if(!p || cycle.Selected()==Stance::Standing || !g_pPlayerManager)return false;
        const int slot=g_pPlayerManager->getSubWeaponEquipped();
        const bool held=(GetAsyncKeyState('C')&0x8000)||((p->m_CurrentInput.m_On|g_dbPad.m_On)&StealthStancePolicy::ButtonBits(p->m_ButtonUseSubweapon));
        return (slot==11 && KunaiSubweapon::Get().IsSmgActive() && KunaiSubweapon::GetSmg().UsesReadyPose()) || ((held || NativeWeaponAction(p->m_Rno0)) && slot>=1 && slot<=6);
    }
    bool Ground(Pl0000* p) const {return BaseGround(p);}
    bool Load() {
        if(attempted)return ready;
        attempted=true;
        const char* names[]={"crouch_idle.gwm","crouch_walk.gwm","prone_idle.gwm","prone_walk.gwm",
            "dive_forward.gwm","dive_fall.gwm","roll_left_start.gwm","roll_left.gwm","roll_left_end.gwm",
            "roll_right_start.gwm","roll_right.gwm","roll_right_end.gwm",
            "crouch_rifle.gwm","prone_rifle.gwm","crouch_rpg.gwm","standing_grenade.gwm","prone_grenade.gwm",
            "cqc_miss.gwm","cqc_combo1.gwm","cqc_combo2.gwm","cqc_combo3.gwm","cqc_combo4.gwm",
            "cqc_grab.gwm","cqc_hold.gwm","cqc_low_grab.gwm","cqc_low_hold.gwm",
            "cqc_throw_f.gwm","cqc_throw_b.gwm","cqc_throw_l.gwm","cqc_throw_r.gwm","cqc_knife.gwm","cqc_low_knife.gwm","prone_to_crouch.gwm","crouch_grenade.gwm"};
        auto root=SamConfig::GetIniPath();root.resize(root.find_last_of("\\/")+1);root+="scripts\\StealthAnims\\";
        for(unsigned i=0;i<34;++i) {
            FILE* file=nullptr;
            if(fopen_s(&file,(root+names[i]).c_str(),"rb")||!file)return false;
            fseek(file,0,SEEK_END);long length=ftell(file);rewind(file);
            if(length<44 || length>16*1024*1024){fclose(file);return false;}
            std::vector<unsigned char> data(length);
            const bool ok=fread(data.data(),1,data.size(),file)==data.size();fclose(file);
            if(!ok || !clips[i].Load(std::move(data)))return false;
        }
        ready=true;return true;
    }
    void RestorePose() {
        Entity* actor=visualOwner.getEntity();
        for(auto& s:saved)
            if(actor && actor->m_pBehavior && actor->m_pBehavior->getPartsPtr(s.bone)==s.part)
            {s.part->m_LocalMatrix=s.matrix;s.part->m_PartsFlag.m_Flag=s.flags;}
        saved.clear();visualOwner=EntityHandle{};
    }
    bool MotionActive(Pl0000* p) const {return p==owner && motion!=StealthMotionPolicy::Move::None && p->m_Rno0==StealthMotionPolicy::Action;}
    void EndMotion(Pl0000* p,bool completed) {
        if(MotionActive(p)) {
            const bool air=p->m_pCharacterControl&&!p->m_pCharacterControl->isOnGround();
            const bool sam=SamNativeRuntime::Get().Active(p);
            p->setRno(sam?(air?0x100005u:0x100000u):(air?0xBu:0u),0,0,0);
        }
        motion=StealthMotionPolicy::Move::None;motionPhase=0;motionTicks=0;current=-1;
        if(completed)cycle.Set(Stance::Prone);
        status=StealthStancePolicy::Name(cycle.Selected());
    }
    void BeginMotion(Pl0000* p,StealthMotionPolicy::Move move) {
        motion=move;motionTicks=motionStage=0;motionPhase=0;motionYaw=p->m_Rot.y;
        if(move==StealthMotionPolicy::Move::Dive) {
            const unsigned on=g_dbPad.m_On|p->m_CurrentInput.m_On;
            float sx=0,sy=0;
            if((on&Hw::PAD_BTN_LAL)||(GetAsyncKeyState('A')&0x8000))sx-=1;
            if((on&Hw::PAD_BTN_LAR)||(GetAsyncKeyState('D')&0x8000))sx+=1;
            if((on&Hw::PAD_BTN_LAU)||(GetAsyncKeyState('W')&0x8000))sy+=1;
            if((on&Hw::PAD_BTN_LAD)||(GetAsyncKeyState('S')&0x8000))sy-=1;
            const auto& camera=g_GameCamera.m_NowMatrix;
            const auto direction=StealthMotionPolicy::DiveDirection(motionYaw,
                camera.m_Target.x-camera.m_Trans.x,camera.m_Target.z-camera.m_Trans.z,sx,sy);
            motionDirection=Hw::cVec4(direction.x,0,direction.z,0);
            motionYaw=std::atan2(direction.x,direction.z);
            status="Dolphin dive";
        } else {
            const float side=move==StealthMotionPolicy::Move::RollLeft?-1.0f:1.0f;
            motionDirection=Hw::cVec4(std::cos(motionYaw)*side,0,-std::sin(motionYaw)*side,0);
            status=side<0?"Tactical roll left":"Tactical roll right";
        }
        auto* states=reinterpret_cast<void**>(reinterpret_cast<char*>(p)+0x7CC);
        if(states[0]&&states[1])reinterpret_cast<void(__thiscall*)(void*,void*)>(shared::base+0x982DE0)(states[0],states[1]);
        p->setRno(StealthMotionPolicy::Action,0,0,0);
    }
    void UpdateDetection(Pl0000* p) {
        detectionStance=0;
        if(!p)return;
        const unsigned noise=StealthStancePolicy::ButtonBits(p->m_ButtonLightAttack|p->m_ButtonHeavyAttack|p->m_ButtonUseItem|p->m_ButtonNinjarun);
        const bool noisy=((p->m_CurrentInput.m_On|g_dbPad.m_On)&noise)!=0 ||
            (GetAsyncKeyState('C')&0x8000) || p->m_Hp<lastHp;
        lastHp=p->m_Hp;
        revealTicks=StealthDetectionPolicy::UpdateReveal(revealTicks,noisy,Paused());
        detectionStance=unsigned(StealthDetectionPolicy::Profile(cycle.Selected(),Ground(p)||MotionActive(p),
            p->isAlive()!=FALSE,Paused(),Scripted(),revealTicks));
    }
public:
    static StealthStanceController& Get() {static StealthStanceController c;return c;}
    const char* Status() const {return status.load();}
    Stance SelectedStance() const {return cycle.Selected();}
    unsigned ClipFrames(int index) {return index>=0 && index<34 && Load()?clips[index].frames:0;}
    void CombatPose(int index,float frame) {combatClip=index;combatFrame=frame;}
    void QuietCqcProfile() {if(!revealTicks && !Paused() && !Scripted())detectionStance=unsigned(cycle.Selected());}
    void Reveal() {revealTicks=120;detectionStance=0;}
    bool ApplyLowGunPose(Pl0000* p) {
        if(!WeaponPose(p) || !Ground(p) || Paused())return false;
        PostTick(p);return poseApplied;
    }
    void FilterNativeInput(Pl0000* p) {
        if(p!=owner || !SamPlayerIdentity::Eligible(p) || Paused() || Scripted() || !p->isAlive() || p->isBladeModeActive())return;
        const unsigned mask=StealthStancePolicy::ButtonBits(p->m_ButtonJump);
        const bool down=((p->m_CurrentInput.m_On|p->m_CurrentInput.m_Trig)&mask)!=0;
        SampleJump(p,down);
        // Filter the live snapshot after collection. Direction modifier bits
        // and standing Jump are preserved; low-stance Jump never reaches graph dispatch.
        StealthInputPolicy::FilterJump(p->m_CurrentInput,p->m_ButtonJump,cycle.Selected()!=Stance::Standing);
    }
    void SetNativeWeaponReleaseReady(bool enabled){nativeReleaseReady=enabled;}
    static void RefreshNativeWeapon(Pl0000* p) {
        (p->m_pParent?p->m_pParent:p)->updateHierarchicalTransformations();
        using Refresh=void(__thiscall*)(void*);
        if(p->m_ppConstraints)reinterpret_cast<Refresh>(shared::base+0x69CCB0)(p->m_ppConstraints);
        Entity* weapon=p->m_SubWep2Handle.getEntity();
        if(weapon && weapon->m_pBehavior) {
            auto* actor=weapon->m_pBehavior;(actor->m_pParent?actor->m_pParent:actor)->updateHierarchicalTransformations();
            if(actor->m_ppConstraints)reinterpret_cast<Refresh>(shared::base+0x69CCB0)(actor->m_ppConstraints);
        }
    }
    bool BeginNativeWeaponRelease(Pl0000* p) {
        if(!releaseSaved.empty() || p!=owner || !nativeReleaseReady || !ready || !Ground(p) || Paused() || cycle.Selected()==Stance::Standing ||
            !g_pPlayerManager || !StealthStancePolicy::NativeWeaponSlot(g_pPlayerManager->getSubWeaponEquipped()))return false;
        const int index=StealthStancePolicy::BaseClip(cycle.Selected(),current==1 || current==3);
        const auto& clip=clips[index];
        D3DXMATRIX basis,inverse;D3DXMatrixRotationY(&basis,p->m_Rot.y);
        basis._41=p->m_TransPos.x;basis._42=p->m_TransPos.y;basis._43=p->m_TransPos.z;
        if(!D3DXMatrixInverse(&inverse,nullptr,&basis))return false;
        std::vector<StealthAdditivePose::Node> nodes;std::vector<D3DXMATRIX> native,result;std::vector<cParts*> parts;
        for(const auto& track:clip.tracks) {
            auto* part=p->getPartsPtr(track.bone);if(!part)continue;
            StealthAdditivePose::Node node;node.bone=track.bone;
            if(!SampleMatrix(clip,track.bone,current==index?phase:0,node.base))return false;
            node.reference=node.aim=node.base;
            D3DXMATRIX world,relative;std::memcpy(&world,&part->m_LocalMatrix,64);D3DXMatrixMultiply(&relative,&world,&inverse);
            nodes.push_back(node);native.push_back(relative);parts.push_back(part);
        }
        for(size_t i=0;i<nodes.size();++i){auto parent=std::find(parts.begin(),parts.end(),parts[i]->m_pPartsParent);nodes[i].parent=parent==parts.end()?-1:int(parent-parts.begin());}
        if(!StealthAdditivePose::ApplyNativeArms(nodes,native,result))return false;
        releaseOwner=p->m_pEntity;
        for(size_t i=0;i<parts.size();++i) {
            auto* part=parts[i];releaseSaved.push_back({short(nodes[i].bone),part,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            D3DXMATRIX world;D3DXMatrixMultiply(&world,&result[i],&basis);std::memcpy(&part->m_LocalMatrix,&world,64);part->m_PartsFlag.m_Flag|=4;
        }
        RefreshNativeWeapon(p);return true;
    }
    void EndNativeWeaponRelease(Pl0000* p) {
        Entity* actor=releaseOwner.getEntity();
        for(const auto& s:releaseSaved)if(actor && actor->m_pBehavior && actor->m_pBehavior->getPartsPtr(s.bone)==s.part){s.part->m_LocalMatrix=s.matrix;s.part->m_PartsFlag.m_Flag=s.flags;}
        releaseSaved.clear();releaseOwner=EntityHandle{};
        if(actor && actor->m_pBehavior==p)RefreshNativeWeapon(p);
    }
    Stance DetectionStance() const {return static_cast<Stance>(detectionStance.load());}
    bool HandleAction(Pl0000* p) {
        if(p!=owner || motion==StealthMotionPolicy::Move::None)return false;
        if(!MotionActive(p)) {EndMotion(p,false);return false;}
        if(Scripted()||!p->isAlive()||p->isBladeModeActive()||!p->m_pCharacterControl||
            motionTicks>180) {EndMotion(p,false);return false;}
        if(Paused())return true;
        const bool grounded=p->m_pCharacterControl->isOnGround();
        const bool dive=motion==StealthMotionPolicy::Move::Dive;
        if(!dive && !grounded && motionTicks>6) {EndMotion(p,false);return false;}
        const float speed=StealthMotionPolicy::Speed(motion,motionStage,motionTicks);
        Hw::cVec4 velocity=motionDirection;velocity.x*=speed;velocity.z*=speed;
        velocity.y=dive?StealthMotionPolicy::Vertical(motionTicks,grounded):-.015f;
        p->m_TransSpeed=velocity;p->field_1020=velocity;
        if(p->m_pCharacterControl->m_pVelocity)*p->m_pCharacterControl->m_pVelocity=velocity;
        p->m_Rot.y=motionYaw;
        reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(p,1,1);
        ++motionTicks;motionPhase+=dive?1.8f:1.6f;
        const auto& clip=clips[StealthMotionPolicy::Clip(motion,motionStage)];
        if(motionPhase>=float(clip.frames-1)) {
            if(dive) {
                if(grounded)EndMotion(p,true);
                else {motionStage=1;motionPhase=(std::min)(motionPhase,float(clips[5].frames-1));}
            } else if(motionStage<2) {++motionStage;motionPhase=0;}
            else EndMotion(p,true);
        }
        return true;
    }
    void RestoreInputs() {
        jumpLatch.End();
        if(jumpReserved) {
            // Device state is restored, but the native snapshot must stay
            // filtered. Restoring its old Jump edge can resurrect a jump in
            // an outer update after TickGame returns.
            g_Keyboard.m_pOn[jumpKeyWord]=(g_Keyboard.m_pOn[jumpKeyWord]&~jumpKeyMask)|jumpKeyOn;
            g_Keyboard.m_pTrig[jumpKeyWord]=(g_Keyboard.m_pTrig[jumpKeyWord]&~jumpKeyMask)|jumpKeyTrig;
            g_Keyboard.m_pRep[jumpKeyWord]=(g_Keyboard.m_pRep[jumpKeyWord]&~jumpKeyMask)|jumpKeyRep;
            jumpReserved=false;
        }
        if(!reserved)return;
        const unsigned mask=reserveMask;
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|padOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|padTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|padRep;
        reserved=false;reserveMask=padOn=padTrig=padRep=0;
    }
    void Forget(Pl0000* player) {
        if(owner!=player)return;
        EndNativeWeaponRelease(player);RestorePose();RestoreInputs();aimWeight=0;owner=nullptr;cycle.Reset();samples.clear();current=-1;positionValid=false;
        motion=StealthMotionPolicy::Move::None;detectionStance=0;revealTicks=0;jumpLatch.Reset();
        status="Standing";
    }
    void BeforeTick(Pl0000* player) {
        RestorePose();RestoreInputs();positionValid=false;poseApplied=false;combatClip=-1;
        if(owner!=player) {owner=player;cycle.Reset();samples.clear();current=-1;phase=0;motion=StealthMotionPolicy::Move::None;
            jumpLatch.Reset();aimWeight=0;revealTicks=0;lastHp=player?player->m_Hp:0;}
        detectionStance=0;
        if(!SamPlayerIdentity::Eligible(player)) {status="Standing";return;}
        const bool allowed=Ground(player)&&!Paused();
        const unsigned on=g_dbPad.m_On,trig=g_dbPad.m_Trig;
        const unsigned jumpKey=unsigned((std::clamp)(SamConfig::GetInt("Stealth","JumpKey",VK_SPACE),0,255));
        const bool jumpDown=((on|trig|player->m_CurrentInput.m_On|player->m_CurrentInput.m_Trig)&StealthStancePolicy::ButtonBits(player->m_ButtonJump))!=0 || (GetAsyncKeyState(int(jumpKey))&0x8000);
        jumpLatch.Begin();SampleJump(player,jumpDown);
        // Sample all rejected presses too; held input never changes stance later.
        const bool stanceKey=(GetAsyncKeyState('G')&0x8000)!=0;
        const bool keyEdge=stanceKey&&!previousStanceKey;previousStanceKey=stanceKey;
        const bool changed=cycle.Update(on|(stanceKey?StealthStancePolicy::R3:0),trig|(keyEdge?StealthStancePolicy::R3:0),allowed&&Load());
        if(changed)status=StealthStancePolicy::Name(cycle.Selected());
        else if(!ready && ((on|trig)&StealthStancePolicy::R3))status="Stance animation bundle missing or invalid";
        if(allowed && ready && !((on|trig)&StealthStancePolicy::L3)) {
            ReservePad(StealthStancePolicy::R3);
        }
        // Request dive/roll from the real input first, then remove Jump from
        // native pad + keyboard collection. No ordinary low-stance jump fallback.
        if(!Paused() && !Scripted() && player->isAlive() && !player->isBladeModeActive() &&
            cycle.Selected()!=Stance::Standing)BlockJump(player);
        before=player->m_TransPos;positionValid=allowed;
        UpdateDetection(player);
    }
    void PostTick(Pl0000* player) {
        if(player!=owner || !ready || poseApplied)return;
        const bool action=MotionActive(player);
        if(Paused() || (!Ground(player)&&!action&&combatClip<0)) {samples.clear();current=-1;return;}
        const Stance stance=cycle.Selected();
        float distance=0;
        if(positionValid&&!action) {
            auto pos=player->m_TransPos;
            const float dx=pos.x-before.x,dz=pos.z-before.z;
            distance=std::hypot(dx,dz);
            // Ignore teleports/scene relocations. Native collision resolved the
            // original step; keep a shorter point along that same ground step.
            if(distance<1.0f) {
                const float factor=StealthStancePolicy::MovementScale(stance,distance);
                if(factor<1 && player->m_pCharacterControl) {
                    pos.x=before.x+dx*factor;pos.z=before.z+dz*factor;
                    player->setTransPos(pos);player->m_pCharacterControl->setPosition(pos,TRUE);
                    player->m_pEntity->setTransPos(pos);
                    player->m_TransSpeed.x*=factor;player->m_TransSpeed.z*=factor;
                    distance*=factor;
                }
            } else {samples.clear();current=-1;return;}
        }
        const bool moving=distance>.0002f;
        const bool weapon=WeaponPose(player)&&!action&&combatClip<0;
        const int slot=g_pPlayerManager?g_pPlayerManager->getSubWeaponEquipped():0;
        const bool nativeAim=weapon && StealthStancePolicy::NativeWeaponSlot(slot);
        if(nativeAim && !nativeReleaseReady){samples.clear();current=-1;return;}
        // MGSV has no prone RPG clip. Its prone rifle brace is the fallback.
        const int weaponClip=slot==11 ? (stance==Stance::Crouch?12:13) : slot>=5 ?
            (stance==Stance::Crouch?14:13) : (stance==Stance::Crouch?33:16);
        const int wanted=combatClip>=0?combatClip:action?StealthMotionPolicy::Clip(motion,motionStage):StealthStancePolicy::BaseClip(stance,moving);
        if(wanted!=current) {
            current=wanted;phase=0;blend=0;
            for(auto& s:samples)s.from=s.last;
        }
        if(current<0 && samples.empty())return;
        D3DXMATRIX basis,inverse;D3DXMatrixRotationY(&basis,player->m_Rot.y);
        basis._41=player->m_TransPos.x;basis._42=player->m_TransPos.y;basis._43=player->m_TransPos.z;
        if(!D3DXMatrixInverse(&inverse,nullptr,&basis))return;
        auto& clip=clips[current<0?0:current];
        const float delta=moving&&clip.stride>0?distance*float(clip.frames-1)/clip.stride:1.0f;
        phase=combatClip>=0?(std::min)(combatFrame,float(clip.frames-1)):action?(std::min)(motionPhase,float(clip.frames-1)):std::fmod(phase+delta,float(clip.frames-1));
        if(action || combatClip>=0 || stance==Stance::Standing || nativeAim)aimWeight=0;
        else aimWeight=(std::clamp)(aimWeight+(weapon?.125f:-.125f),0.0f,1.0f);
        if(weapon) {
            lastAimClip=weaponClip;
            lastAimFrame=slot>=5 || slot==11 ? (weaponClip==13?13.0f:12.0f) :
                player->m_Rno0==0x38 && player->m_Rno1>=5 ? (std::min)(float(player->m_AnimationFrame)*1.8f,float(clips[weaponClip].frames-1)) : 0.0f;
        }
        const unsigned lo=unsigned(phase),hi=(std::min)(lo+1,clip.frames-1);
        const unsigned blendFrames=action?4u:12u;
        const float amount=float((std::min)(blendFrames,++blend))/float(blendFrames);
        visualOwner=player->m_pEntity;
        std::vector<StealthAdditivePose::Node> layers;
        std::vector<cParts*> parts;
        std::vector<D3DXMATRIX> nativeArms;
        for(const auto& track:clip.tracks) {
            auto* part=player->getPartsPtr(track.bone);if(!part)continue;
            saved.push_back({track.bone,part,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            D3DXMATRIX native,relative;
            std::memcpy(&native,&part->m_LocalMatrix,sizeof(native));
            D3DXMatrixMultiply(&relative,&native,&inverse);
            nativeArms.push_back(relative);
            auto found=std::find_if(samples.begin(),samples.end(),[&](const Sample& s){return s.bone==track.bone;});
            if(found==samples.end()) {samples.push_back({track.bone,relative,relative});found=samples.end()-1;}
            if(current>=0) {
                D3DXMATRIX a,b;
                std::memcpy(&a,clip.bytes.data()+track.offset+lo*64,64);
                std::memcpy(&b,clip.bytes.data()+track.offset+hi*64,64);
                if(!GunAimMath::BlendMatrix(a,b,phase-lo,relative)){nativeArms.pop_back();continue;}
            }
            D3DXMATRIX blended;
            if(GunAimMath::BlendMatrix(found->from,relative,amount,blended))relative=blended;
            found->last=relative;
            StealthAdditivePose::Node node;node.bone=track.bone;node.base=relative;
            node.reference=relative;node.aim=relative;
            if(aimWeight>0) {
                if(!SampleMatrix(clips[stance==Stance::Crouch?0:2],track.bone,0,node.reference) ||
                    !SampleMatrix(clips[lastAimClip],track.bone,lastAimFrame,node.aim))aimWeight=0;
            }
            layers.push_back(node);parts.push_back(part);
        }
        for(size_t i=0;i<layers.size();++i) {
            auto parent=std::find(parts.begin(),parts.end(),parts[i]->m_pPartsParent);
            layers[i].parent=parent==parts.end()?-1:int(parent-parts.begin());
        }
        std::vector<D3DXMATRIX> layered;
        const bool layeredOk=nativeAim?StealthAdditivePose::ApplyNativeArms(layers,nativeArms,layered):StealthAdditivePose::Apply(layers,aimWeight,layered);
        if(!layeredOk) {
            layered.clear();for(const auto& n:layers)layered.push_back(n.base);
        }
        for(size_t i=0;i<layers.size();++i) {
            D3DXMATRIX world;D3DXMatrixMultiply(&world,&layered[i],&basis);
            std::memcpy(&parts[i]->m_LocalMatrix,&world,sizeof(world));parts[i]->m_PartsFlag.m_Flag|=4;
        }
        poseApplied=true;
        (player->m_pParent?player->m_pParent:player)->updateHierarchicalTransformations();
        if(player->m_ppConstraints)reinterpret_cast<void(__thiscall*)(void*)>(shared::base+0x69CCB0)(player->m_ppConstraints);
        if(nativeAim)RefreshNativeWeapon(player);
        if(current<0 && blend>=12)samples.clear();
    }
};
