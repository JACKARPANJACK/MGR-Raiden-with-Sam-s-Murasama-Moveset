#pragma once
#include "MonsoonKickPolicy.h"
#include "SamAttackInputPolicy.h"
#include "SamArchiveLookup.h"
#include "WeaponSwitcher.h"
#include "SaiPullPolicy.h"
#include "SaiPullController.h"
#include "GunRuntimeLog.h"
#include "MonsoonKickEffects.h"
#include <vector>
#include <cstdio>
#include <cstring>

class MonsoonKickController {
    Pl0000* owner = nullptr;
    unsigned ticks = 0;
    SamAttackInputPolicy::Reader input;
    SamAttackInputPolicy::Reader cancelInput;
    MonsoonKickPolicy::CancelBuffer cancel;
    bool sampled=false,handedOff=false,hitConfirmed=false,previousPullKey=false;
    unsigned previousPullPad=0,reservedMask=0;
    unsigned savedOn=0,savedTrig=0,savedRep=0,savedPadOn=0,savedPadTrig=0,savedPadRep=0;
    std::vector<char> motion, sequence;
    struct Target {EntityHandle entity{};int hp=0;bool launched=false;};
    std::vector<Target> targets;
    int attackIdentity=-1;
    unsigned preAction=0;
    bool preAir=false;
    bool LoadFile(const char* name, std::vector<char>& bytes) {
        std::string path = SamConfig::GetIniPath();
        path.resize(path.find_last_of("\\/")+1);
        path += "scripts\\BaseMoveset\\"; path += name;
        FILE* file = nullptr;
        if (fopen_s(&file,path.c_str(),"rb") || !file) return false;
        fseek(file,0,SEEK_END);const long size=ftell(file);rewind(file);
        if(size<16 || size>8*1024*1024) {fclose(file);return false;}
        bytes.resize(size);const bool ok=fread(bytes.data(),1,size,file)==size;
        fclose(file);if(!ok)bytes.clear();return ok;
    }
    bool Load() {
        if(!motion.empty() && !sequence.empty()) return true;
        if(!LoadFile("monsoon_kick.mot",motion) || !LoadFile("monsoon_kick_seq.bxm",sequence) ||
           std::memcmp(motion.data(),"mot\0",4) ||
           (std::memcmp(sequence.data(),"XML\0",4) && std::memcmp(sequence.data(),"BXM\0",4))) {
            motion.clear();sequence.clear();return false;
        }
        return true;
    }
    bool SupportedWeapon() const {
        return g_pPlayerManager && MonsoonKickPolicy::WeaponAllowed(WeaponSwitcher::Get().Selected(),
            g_pPlayerManager->getCustomWeaponEquipped(),SamMovesetManager::Instance().IsEnabled());
    }
    bool SaiEquipped() const {
        return g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()==3;
    }
    void ShowSai(Pl0000* player) {
        if(!SaiEquipped())return;
        auto* weapon=player->m_CustomWeaponHandle.getEntity();if(!weapon || !weapon->m_pBehavior)return;
        // The custom action is outside native Sai draw-state ranges. Preserve
        // the existing slot-5 attachment and its native render opacity.
        player->field_1428=1.0f;
        auto* behavior=weapon->m_pBehavior;
        behavior->onDisp();
        for(int n=0;behavior->m_pMeshes && n<behavior->m_MeshNum;++n) {
            behavior->m_pMeshes[n].m_MeshFlags|=1u;
            behavior->m_pMeshes[n].m_Color.a=1;
            behavior->m_pMeshes[n].m_ColorScale.a=1;
        }
    }
public:
    static MonsoonKickController& Get() {static MonsoonKickController value;return value;}
    bool Active(Behavior* actor) const {
        return owner && actor==owner && owner->m_Rno0==MonsoonKickPolicy::Action && SupportedWeapon();
    }
    SamArchiveLookup::Clip Clip() {return {motion.data(),sequence.data(),SamArchiveLookup::Source::Playable};}
    void ReserveCancelInputs(Pl0000* player) {
        reservedMask=(player->m_ButtonLightAttack|player->m_ButtonHeavyAttack|player->m_ButtonJump)&~0xFu;
        reservedMask|=SaiPullPolicy::LeftStickButton;
        savedOn=player->m_CurrentInput.m_On&reservedMask;savedTrig=player->m_CurrentInput.m_Trig&reservedMask;savedRep=player->m_CurrentInput.m_Rep&reservedMask;
        savedPadOn=g_dbPad.m_On&reservedMask;savedPadTrig=g_dbPad.m_Trig&reservedMask;savedPadRep=g_dbPad.m_Rep&reservedMask;
        player->m_CurrentInput.m_On&=~reservedMask;player->m_CurrentInput.m_Trig&=~reservedMask;player->m_CurrentInput.m_Rep&=~reservedMask;
        g_dbPad.m_On&=~reservedMask;g_dbPad.m_Trig&=~reservedMask;g_dbPad.m_Rep&=~reservedMask;
    }
    void RestoreCancelInputs(Pl0000* player) {
        if(!reservedMask)return;
        player->m_CurrentInput.m_On=(player->m_CurrentInput.m_On&~reservedMask)|savedOn;
        player->m_CurrentInput.m_Trig=(player->m_CurrentInput.m_Trig&~reservedMask)|savedTrig;
        player->m_CurrentInput.m_Rep=(player->m_CurrentInput.m_Rep&~reservedMask)|savedRep;
        g_dbPad.m_On=(g_dbPad.m_On&~reservedMask)|savedPadOn;
        g_dbPad.m_Trig=(g_dbPad.m_Trig&~reservedMask)|savedPadTrig;
        g_dbPad.m_Rep=(g_dbPad.m_Rep&~reservedMask)|savedPadRep;
        reservedMask=0;
    }
    void Forget(Pl0000* player) {if(owner==player){RestoreCancelInputs(player);cancel.Clear();cancelInput.Reset();MonsoonKickEffects::Get().Stop();owner=nullptr;ticks=0;input.Reset();targets.clear();}}
    // Called before native action selection, after CurrentInput collection.
    void SampleNativeInput(Pl0000* player) {
        if(player!=owner || sampled)return;sampled=true;
        const auto attack=cancelInput.Poll(player->m_CurrentInput.m_On|g_dbPad.m_On,
            player->m_CurrentInput.m_Trig|g_dbPad.m_Trig,player->m_ButtonLightAttack,player->m_ButtonHeavyAttack);
        const bool key=(GetAsyncKeyState('F')&0x8000)!=0;
        const bool pull=SaiPullPolicy::Press(key,previousPullKey,g_dbPad.m_On,g_dbPad.m_Trig,previousPullPad);
        previousPullKey=key;previousPullPad=g_dbPad.m_On|g_dbPad.m_Trig;
        if(!Active(player)){cancel.Clear();return;}
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        if(!player->isAlive() || player->isInAir() || player->isBladeModeActive() || gui::IsMenuVisible() ||
           foreground!=GetCurrentProcessId() || g_StaFlags.STA_PAUSE || g_StaFlags.STA_QTE ||
           g_StaFlags.STA_EVENT || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT){cancel.Clear();return;}
        const auto command=MonsoonKickPolicy::Command(attack.light,attack.heavy,
            (player->m_CurrentInput.m_Trig&player->m_ButtonJump)!=0,pull);
        const auto next=cancel.Poll(float(player->m_AnimationFrame),hitConfirmed,command,Trigger::StpFlags.STP_OBJ);
        if(next!=MonsoonKickPolicy::FollowUp::None) {
            ReserveCancelInputs(player);
            handedOff=true;ticks=0;MonsoonKickEffects::Get().Stop();
            player->setRno(0,0,0,0);
            unsigned replay=0;
            using FollowUp=MonsoonKickPolicy::FollowUp;
            if(next==FollowUp::Light || next==FollowUp::Chord)replay|=player->m_ButtonLightAttack&~0xFu;
            if(next==FollowUp::Heavy || next==FollowUp::Chord)replay|=player->m_ButtonHeavyAttack&~0xFu;
            if(next==FollowUp::Jump)replay|=player->m_ButtonJump&~0xFu;
            player->m_CurrentInput.m_On|=replay;player->m_CurrentInput.m_Trig|=replay;
            g_dbPad.m_On|=replay;g_dbPad.m_Trig|=replay;
            if(next==FollowUp::Pull)SaiPullController::Get().QueuePull();
            MonsoonKickPolicy::Status="Monsoon kick cancelled into a follow-up";
            GunRuntimeLog::Write("[MonsoonKick] native follow-up handoff command=%u frame=%d",unsigned(next),player->m_AnimationFrame);
            return;
        }
        // Keep held and early buffered attacks from replacing the kick before
        // its hit window. Restore both input sources after native selection.
        ReserveCancelInputs(player);
    }
    void AfterNativeInput(Pl0000* player) {
        if(player==owner)RestoreCancelInputs(player);
    }
    void BeforeTick(Pl0000* player) {
        sampled=false;handedOff=false;
        if(!player) {targets.clear();return;}
        preAction=player->m_Rno0;
        preAir=player->isInAir()!=FALSE;
        if(SupportedWeapon())MonsoonKickEffects::Get().Update();
        if(!Active(player)) {cancel.Clear();targets.clear();MonsoonKickEffects::Get().Stop();return;}
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        for(int n=0;n<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++n,++it) {
            auto* entity=*it;
            if(!SamTargets::Enemy(entity) || !SaiPullPolicy::Eligible(unsigned(entity->m_ObjId)))continue;
            auto found=std::find_if(targets.begin(),targets.end(),[entity](const Target& t){return t.entity.getEntity()==entity;});
            const int hp=static_cast<BehaviorAppBase*>(entity->m_pBehavior)->m_Hp;
            if(found==targets.end()){Target target;target.entity=entity;target.hp=hp;targets.push_back(target);}
            else found->hp=hp;
        }
    }
    void PostTick(Pl0000* player) {
        if(!Active(player) || Trigger::StpFlags.STP_OBJ)return;
        ShowSai(player);
        if(attackIdentity<0)return;
        for(auto& target:targets) {
            auto* entity=target.entity.getEntity();if(!SamTargets::Enemy(entity))continue;
            auto* actor=static_cast<BehaviorAppBase*>(entity->m_pBehavior);
            const auto& hits=actor->m_AttackHits;bool owned=false;
            if(hits.m_vector && hits.m_size<=hits.m_capacity && hits.m_size<=128)
                for(size_t n=0;n<hits.m_size;++n) {
                    const auto& hit=hits.m_vector[n].m_HitData;
                    if(hit.field_0==attackIdentity && hit.field_18.getEntity()==player->m_pEntity){owned=true;break;}
                }
            if(!MonsoonKickPolicy::ConfirmedLaunch(target.hp,actor->m_Hp,owned,
                float(player->m_AnimationFrame),target.launched))continue;
            // A confirmed native collision supplies damage. Apply an upward
            // physical impulse once so grounded enemy AI cannot flatten it.
            actor->m_TransSpeed.y=(std::max)(actor->m_TransSpeed.y,0.48f);
            auto pos=entity->getTransPos();pos.y+=0.25f;
            actor->setTransPos(pos);
            if(actor->m_pCharacterControl)actor->m_pCharacterControl->setPosition(pos,TRUE);
            entity->setTransPos(pos);target.launched=true;
            hitConfirmed=true;
            MonsoonKickPolicy::Status="Kick hit: enemy launched; Jump for an air combo";
            GunRuntimeLog::Write("[MonsoonKick] confirmed launch target=%X frame=%d",unsigned(entity->m_ObjId),player->m_AnimationFrame);
        }
    }
    bool HandleAction(Pl0000* player) {
        if(owner!=player) {owner=player;ticks=0;input.Reset();}
        const bool active=player->m_Rno0==MonsoonKickPolicy::Action;
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool scripted=g_StaFlags.STA_QTE||g_StaFlags.STA_EVENT||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT;
        const bool blocked=scripted||g_StaFlags.STA_PAUSE||gui::IsMenuVisible()||
            foreground!=GetCurrentProcessId()||Trigger::StpFlags.STP_OBJ;
        if(active) {
            if(!SupportedWeapon() || !player->isAlive() || scripted || player->isBladeModeActive() || player->isInAir()) {
                cancel.Clear();MonsoonKickEffects::Get().Stop();ticks=0;player->setRno(player->isInAir()?0xB:0,0,0,0);return false;
            }
            if(blocked) return true;
            if(!sampled && MonsoonKickPolicy::CanJumpCancel(float(player->m_AnimationFrame),
                (player->m_CurrentInput.m_Trig&player->m_ButtonJump)!=0)) {
                MonsoonKickEffects::Get().Stop();ticks=0;player->setRno(9,0,0,0);return false;
            }
            // The same native motion update used by other custom attacks;
            // the paired sequence owns collision and launcher damage.
            reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(player,1,1);
            if(++ticks>240 || player->getRemainingAnimationFrames(0)<=0) {
                cancel.Clear();MonsoonKickEffects::Get().Stop();player->setRno(0,0,0,0);ticks=0;
            }
            return true;
        }
        const auto attack=input.Poll(player->m_CurrentInput.m_On|g_dbPad.m_On,
            player->m_CurrentInput.m_Trig|g_dbPad.m_Trig,
            player->m_ButtonLightAttack,player->m_ButtonHeavyAttack);
        if(handedOff)return false; // Let this input select its native Sai move.
        float x=g_dbPad.m_LeftStick.x,y=g_dbPad.m_LeftStick.y;
        const bool a=(GetAsyncKeyState('A')&0x8000)!=0,d=(GetAsyncKeyState('D')&0x8000)!=0;
        const bool w=(GetAsyncKeyState('W')&0x8000)!=0,s=(GetAsyncKeyState('S')&0x8000)!=0;
        if(a||d||w||s){x=float(d)-float(a);y=float(w)-float(s);}
        const unsigned startState=MonsoonKickPolicy::StartState(player->m_Rno0,preAction,player->m_Rno1==0,SaiEquipped());
        const bool airborne=startState!=player->m_Rno0 ? preAir : player->isInAir()!=FALSE;
        if(attack.heavy && SaiEquipped())
            GunRuntimeLog::Write("[MonsoonKick] heavy input state=%X prior=%X phase=%u resolved=%X air=%u preAir=%u light=%u direction=%.2f,%.2f selected=%d",
                player->m_Rno0,preAction,player->m_Rno1,startState,unsigned(airborne),unsigned(preAir),
                unsigned(attack.light||attack.lightHeld),x,y,WeaponSwitcher::Get().Selected());
        if(!MonsoonKickPolicy::CanStart(startState,player->isAlive()!=FALSE,airborne,
            player->isBladeModeActive()!=FALSE,blocked,SupportedWeapon(),attack.heavy,attack.light||attack.lightHeld,x,y) || !Load()) return false;
        if(!MonsoonKickEffects::Get().Update()) {MonsoonKickPolicy::Status="Loading Monsoon kick effects; press Back + Heavy again";return false;}
        player->m_CurrentInput.m_Trig&=~player->m_ButtonHeavyAttack;
        player->setRno(MonsoonKickPolicy::Action,0,0,0);ticks=0;hitConfirmed=false;cancel.Clear();
        attackIdentity=-1;
        const uint16_t number=64;
        auto* table=*reinterpret_cast<uintptr_t**>(player);
        auto* descriptor=reinterpret_cast<CollisionAttackData*(__thiscall*)(Pl0000*,const uint16_t*)>(table[0x130/4])(player,&number);
        if(descriptor) {
            attackIdentity=descriptor->m_CollisionDataContainer.field_0;
            auto* descriptorTable=*reinterpret_cast<uintptr_t**>(descriptor);
            reinterpret_cast<void(__thiscall*)(CollisionAttackData*,unsigned)>(descriptorTable[1])(descriptor,1);
        }
        const int result=player->requestAnimationByName(MonsoonKickPolicy::Code,0,0.08f,1,
            0x8000000,0,1.35f);
        if(result==-1){MonsoonKickPolicy::Status="Kick animation could not start";player->setRno(0,0,0,0);return false;}
        ShowSai(player);
        MonsoonKickEffects::Get().Play(player);
        MonsoonKickPolicy::Status="Kick started: aim the rising foot at a nearby enemy";
        GunRuntimeLog::Write("[MonsoonKick] started prior=%X attack=%d",preAction,attackIdentity);
        return true;
    }
};
