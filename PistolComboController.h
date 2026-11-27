#pragma once
#include "PistolComboPolicy.h"
#ifndef PISTOL_COMBO_CONTROLLER_TEST
#include "PistolAssets.h"
#include "SamArchiveLookup.h"
#include "SamBossSequence.h"
#include "EncounterPolicy.h"
#include "GunRuntimeLog.h"
#endif

class PistolComboController {
    struct Pair {std::vector<unsigned char> motion,sequence,visual;unsigned frames=0,recovery=0;std::vector<unsigned> cues;};
    Pair clips[PistolAttackCatalog::Count];
    bool loaded=false;
    Pl0000* owner=nullptr;
    PistolComboPolicy::Direction direction=PistolComboPolicy::None;
    unsigned clipIndex=0;
    bool bodyPrepared=false;
    bool running=false;
    float comboClock=0,previousFrame=0;
    DWORD endedAt=0;
    PistolComboPolicy::Chain chain;
    PistolComboPolicy::Playback playback;
    unsigned Index() const {return clipIndex;}
    const char* Code() const {return PistolAttackCatalog::Clips[Index()].code;}
    bool Owns(Behavior* p) const {return running && loaded && bodyPrepared && p && p==owner && owner->m_Rno0==PistolComboPolicy::Action;}
    bool Selected() const {return g_pPlayerManager && PistolPolicy::Slot(g_pPlayerManager->getSubWeaponEquipped());}
    bool Load() {
        if(loaded)return true;
        if(!PistolAssets::LoadAnimations())return false;
        for(unsigned i=0;i<PistolAttackCatalog::Count;++i) {
            const char* code=PistolAttackCatalog::Clips[i].code;
            char name[48];std::snprintf(name,sizeof(name),"wp2062_%s.mot",code);
            clips[i].motion=PistolAssets::Member(PistolAssets::animations,name);
            std::snprintf(name,sizeof(name),"wp2062_%s_0_seq.bxm",code);
            clips[i].sequence=PistolAssets::Member(PistolAssets::animations,name);
            std::snprintf(name,sizeof(name),"wp2062_%s.gwm",code);
            clips[i].visual=PistolAssets::Member(PistolAssets::animations,name);
            if(clips[i].visual.size()<44 || std::memcmp(clips[i].visual.data(),"FBW1",4))return false;
            auto checked=clips[i].visual;std::memcpy(checked.data(),"STW1",4);StealthPoseFile::Clip visual;
            if(!visual.Load(std::move(checked)))return false;
            std::snprintf(name,sizeof(name),"wp2062_%s_timing.bin",code);
            auto timing=PistolAssets::Member(PistolAssets::animations,name);
            if(timing.size()<20 || std::memcmp(timing.data(),"PCT2",4) || clips[i].motion.size()<44 ||
                std::memcmp(clips[i].motion.data(),"mot\0",4) || clips[i].sequence.size()<16 ||
                SamBossSequence::Size(clips[i].sequence.data())!=clips[i].sequence.size())return false;
            unsigned count=0;
            std::memcpy(&clips[i].frames,timing.data()+4,4);
            std::memcpy(&clips[i].recovery,timing.data()+8,4);std::memcpy(&count,timing.data()+12,4);
            unsigned short frames=0;std::memcpy(&frames,clips[i].motion.data()+10,2);
            if(frames!=clips[i].frames || frames!=visual.frames || frames<2 || frames>2048 || count!=3 || timing.size()!=16+count*4 ||
                clips[i].recovery>=frames)return false;
            clips[i].cues.resize(count);std::memcpy(clips[i].cues.data(),timing.data()+16,count*4);
            unsigned previous=0;
            for(unsigned frame:clips[i].cues) {
                if(frame<=previous || frame>=clips[i].recovery)return false;
                previous=frame;
            }
        }
        loaded=true;return true;
    }
    bool Start(Pl0000* player) {
        playback.Reset();PistolComboPolicy::shots=0;previousFrame=0;
        if(!bodyPrepared && !KunaiSubweapon::GetSmg().BeginPistolAttack(player,clips[Index()].visual.data())) {
            GunRuntimeLog::Write("pistol full-body capture failed");PistolComboPolicy::input.Cancel();return false;
        }
        const auto& style=PistolAttackCatalog::Clips[Index()];
        bodyPrepared=true;KunaiSubweapon::GetSmg().BeginPistolStage(style.blend);
        running=true;player->setRno(PistolComboPolicy::Action,0,0,0);
        auto& clip=clips[Index()];
        // Direct playback supplies the pair atomically and restores native
        // animation playback multipliers; name lookup can inherit idle state.
        const int node=player->setDirectAnimation(clip.motion.data(),clip.sequence.data(),0,.03f,1,0x8000000,0,style.rate);
        GunRuntimeLog::Write("pistol playback v23 code=%s node=%d frames=%u rate=%.2f remaining=%.2f",Code(),node,clip.frames,style.rate,player->getRemainingAnimationFrames(0));
        if(node==-1) {
            Stop(player);return false;
        }
        return true;
    }
    void Release() {
        if(running)endedAt=GetTickCount();
        KunaiSubweapon::GetSmg().EndPistolAttack();
        PistolComboPolicy::shots=0;direction=PistolComboPolicy::None;playback.Reset();chain.Clear();bodyPrepared=running=false;PistolComboPolicy::input.Cancel();
        comboClock=previousFrame=0;
    }
    void Stop(Pl0000* player) {
        Release();
        // Ending the custom action alone leaves its attack-lock sequence and
        // last pose on node zero. Replace the pair with native idle as well.
        player->m_pSeqFlags[0]&=~0x40000u;
        const bool air=player->isInAir()!=FALSE;
        player->setRno(air?0xB:0,0,0,0);
        if(!air)player->requestAnimationByName("0000",0,.12f,1,0x8000000,0,1);
    }
public:
    static PistolComboController& Get(){static PistolComboController c;return c;}
    bool Active(Behavior* p) const {return Owns(p) && Selected();}
    const void* VisualPose(Pl0000* p,float& frame) {
        if(!Active(p))return nullptr;
        auto& clip=clips[Index()];const float remaining=p->getRemainingAnimationFrames(0);
        if(!std::isfinite(remaining) || remaining<0 || remaining>float(clip.frames)+1)return nullptr;
        frame=(std::clamp)(float(clip.frames-1)-remaining,0.0f,float(clip.frames-1));return clip.visual.data();
    }
    SamArchiveLookup::Clip RequestedClip(Behavior* p,const char* code) {
        if(!Active(p) || !code || std::strcmp(code,Code()))return {};
        auto& clip=clips[Index()];return {clip.motion.data(),clip.sequence.data(),SamArchiveLookup::Source::Playable};
    }
    void Forget(Pl0000* p) {if(p==owner){Release();owner=nullptr;chain.Reset();endedAt=0;}}
    bool HandleAction(Pl0000* player) {
        if(owner!=player){if(running || bodyPrepared)Release();owner=player;chain.Reset();}
        if(running && player->m_Rno0!=PistolComboPolicy::Action) {
            GunRuntimeLog::Write("pistol playback interrupted by native state=%X",player->m_Rno0);
            Release(); // Preserve the native action that replaced our attack.
        }
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        const bool enabled=Selected() && player->isAlive() && !player->isInAir() && !player->isBladeModeActive() &&
            !story && !gui::IsMenuVisible() && foreground==GetCurrentProcessId();
        const auto command=PistolComboPolicy::input.Take();
        const bool active=Owns(player);
        if(active) {
            if(!enabled){Stop(player);return false;}
            if(g_StaFlags.STA_PAUSE || Trigger::StpFlags.STP_OBJ)return true;
            if(PistolComboPolicy::input.fire) {
                Stop(player);PistolComboPolicy::input.ManualHandoff();
                KunaiSubweapon::GetSmg().ResumePistolManual(player,true);
                return false;
            }
            // Jump and melee cancel into the native graph without replaying a shot.
            const unsigned cancel=(player->m_ButtonJump|player->m_ButtonHeavyAttack|player->m_ButtonLightAttack|player->m_ButtonNinjarun)&~0xFu;
            if(player->m_CurrentInput.m_Trig&cancel) {
                Stop(player);return false;
            }
            // Custom actions bypass the native action dispatcher. Run its
            // common animation/weapon maintenance before reading node time.
            // The tail calls Behavior's animation slot/sequence update.
            reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(player,1,1);
            auto& clip=clips[Index()];
            const float remaining=player->getRemainingAnimationFrames(0);
            const float frame=remaining>=0 && remaining<=float(clip.frames)+1?
                (std::max)(0.0f,float(clip.frames-1)-remaining):-1.0f;
            if(std::isfinite(frame) && frame>=previousFrame)comboClock+=(frame-previousFrame)/PistolAttackCatalog::Clips[Index()].rate;
            previousFrame=frame;
            if(command!=PistolComboPolicy::None) {
                chain.Push(command,comboClock);
                GunRuntimeLog::Write("pistol follow-up v23 direction=%u clock=%.2f",unsigned(command),comboClock);
            }
            const auto result=playback.Tick(frame,clip.cues.data(),unsigned(clip.cues.size()),clip.recovery,clip.frames);
            if(result==PistolComboPolicy::Playback::Fire) {
                PistolComboPolicy::shots=playback.due;
                GunRuntimeLog::Write("pistol volley cue code=%s frame=%.2f rounds=%u",Code(),frame,playback.due);
            }
            if(result==PistolComboPolicy::Playback::Failed) {
                GunRuntimeLog::Write("pistol combo aborted v23 code=%s frames=%u frame=%.2f remaining=%.2f ticks=%u stalled=%u",Code(),clip.frames,frame,remaining,playback.ticks,playback.stalled);
                Stop(player);return false;
            }
            // Let PostTick emit the final volley from the outgoing pose before
            // replacing it. A queued follow-up then skips the recovery tail.
            if(result!=PistolComboPolicy::Playback::Fire && frame>=float(clip.cues.back()+2)) {
                PistolComboPolicy::Chain::Step next;
                if(chain.Pop(next,comboClock)){direction=next.direction;clipIndex=next.clip;Start(player);return true;}
            }
            if(result==PistolComboPolicy::Playback::Finished)Stop(player);
            return true;
        }
        PistolComboPolicy::shots=0;
        if(!enabled || g_StaFlags.STA_PAUSE || Trigger::StpFlags.STP_OBJ || command==PistolComboPolicy::None ||
            !(player->m_Rno0<=4 || (player->m_Rno0>=0x100000 && player->m_Rno0<=0x100002))) {
            if(command!=PistolComboPolicy::None)GunRuntimeLog::Write("pistol request blocked v23 state=%X enabled=%u pause=%u stop=%u",player->m_Rno0,unsigned(enabled),unsigned(g_StaFlags.STA_PAUSE),unsigned(Trigger::StpFlags.STP_OBJ));
            return false;
        }
        direction=command;
        if(direction==PistolComboPolicy::None || !KunaiSubweapon::GetSmg().IsEquipped() || !Load()) {
            GunRuntimeLog::Write("pistol request unavailable v23 direction=%u equipped=%u loaded=%u",unsigned(direction),unsigned(KunaiSubweapon::GetSmg().IsEquipped()),unsigned(loaded));
            PistolComboPolicy::input.Cancel();return false;
        }
        if(endedAt && DWORD(GetTickCount()-endedAt)>900)chain.Reset();
        chain.Clear();bodyPrepared=false;comboClock=0;
        for(unsigned i=0;i<PistolComboPolicy::Length(direction);++i)chain.Push(direction);
        PistolComboPolicy::Chain::Step first;if(!chain.Pop(first))return false;
        direction=first.direction;clipIndex=first.clip;
        GunRuntimeLog::Write("pistol directional combo=%u",unsigned(direction));
        return Start(player);
    }
};
