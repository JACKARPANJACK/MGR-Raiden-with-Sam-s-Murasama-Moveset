#pragma once
#include "SmgPolicy.h"
#include "NativeSmgMenuPolicy.h"
#include "SamTargets.h"
#include "SamNativeRuntime.h"
#include "SamCombatRouting.h"
#include "SamUltimatePolicy.h"
#include "SamStagePolicy.h"
#include "EncounterPolicy.h"
#include "gui.h"
#include "SamVisualEffects.h"
#include "NativeGunPose.h"
#include "GunRuntimeLog.h"
#include "WallTraversalPolicy.h"
#include "PistolAssets.h"
#include "SamElectricCombat.h"
#include "PistolSleep.h"
#include <cObjReadManager.h>
#include <Em0010.h>
#include <BehaviorBalkan.h>
#include <cCameraGame.h>
#include <array>
#include <vector>
#include <atomic>
#include <cfloat>

// Enemy gun/model and BulletBase factory; never invoke Em0040 AI on Pl0000.
class SmgSubweapon
{
    struct Resource {unsigned id;bool requested=false,pinned=false,bank=false,registered=false;Hw::cFmerge archive;unsigned retry=0;};
    std::array<Resource,7> resources{{{SmgPolicy::Gun},{SmgPolicy::Bullet},{SmgPolicy::MotionBank},{SmgPolicy::GunBank},{SmgPolicy::BulletPool},{SmgPolicy::VectorProfile.model},{0x31051}}};
    bool vectorGun=false;
    unsigned gunIndex=0;
    bool Pistol() const {return gunIndex>=2;}
    unsigned PistolKind() const {return gunIndex-2;}
    const SmgPolicy::GunProfile& Profile() const {return SmgPolicy::Profile(gunIndex);}
    PistolPolicy::Control pistolControl;
    PistolSleep pistolSleep;
    struct SeenPistolHit {EntityHandle target;unsigned signature=0,frame=0;};
    std::vector<SeenPistolHit> seenPistolHits;
    unsigned impactFrame=0;
    bool NewPistolHit(Entity* target,const cAtHit& hit) {
        // Only count a contact when it first appears. A persistent native
        // hit record must never turn one tranquilizer round into three doses.
        unsigned signature=2166136261u;
        for(const auto* p=reinterpret_cast<const unsigned char*>(&hit);p<reinterpret_cast<const unsigned char*>(&hit)+0x100;++p)
            signature=(signature^*p)*16777619u;
        for(auto& v:seenPistolHits)if(v.target.getEntity()==target && v.signature==signature) {
            const bool fresh=v.frame+1<impactFrame;v.frame=impactFrame;return fresh;
        }
        SeenPistolHit v;v.target=target;v.signature=signature;v.frame=impactFrame;seenPistolHits.push_back(v);return true;
    }
    std::array<std::atomic<unsigned>,2> pistolRounds{{15,12}};
    std::atomic<unsigned> displayGunIndex{0},displayCapacity{30};
    const char* PistolPose() const {
        const unsigned stance=pistolStanceCallback?pistolStanceCallback():0;
        return stance==1?"pistol_crouch.gwm":stance==2?"pistol_prone.gwm":"pistol_ready.gwm";
    }
    void PublishRounds() {
        if(Pistol())pistolRounds[PistolKind()]=fire.rounds;
        else (vectorGun?vectorRounds:rifleRounds)=fire.rounds;
    }
    unsigned Muzzle() const {return Profile().muzzle;}
    Pl0000* owner=nullptr;
    EntityHandle gun;
    EntityHandle bulletPool;
    bool swordParkRequested=false;
    EntityHandle dumpTarget;
    // Only the synchronous pool call made by this controller replaces native
    // ShotEffData emission. Other player/enemy pooled shots stay native.
    inline static thread_local Entity* emittingOwner=nullptr;
    inline static thread_local Entity* emittingWeapon=nullptr;
    inline static thread_local unsigned emittingGun=0;
    static void __fastcall PooledShotEffect(BehaviorBalkan* pool,void*,void* bullet)
    {
        auto* bytes=static_cast<unsigned char*>(bullet);
        auto* shotOwner=reinterpret_cast<EntityHandle*>(bytes+0x360);
        if(emittingOwner && emittingWeapon && shotOwner->getEntity()==emittingOwner)
        {
            unsigned savedOwner=0,savedMode=0;unsigned short savedBone=0;
            float savedOffset[4];
            std::memcpy(&savedOwner,bytes+0x360,4);
            std::memcpy(&savedMode,bytes+0x36C,4);
            std::memcpy(&savedBone,bytes+0x1BA,2);
            std::memcpy(savedOffset,bytes+0x190,16);
            *shotOwner=emittingWeapon;
            const auto& profile=SmgPolicy::Profile(emittingGun);
            const float offset[4]={0,profile.muzzleUp,profile.muzzleForward,0};
            std::memcpy(bytes+0x190,offset,16);
            *reinterpret_cast<unsigned short*>(bytes+0x1BA)=static_cast<unsigned short>(profile.muzzle);
            *reinterpret_cast<unsigned*>(bytes+0x36C)=4;
            __try {
                reinterpret_cast<void(__thiscall*)(BehaviorBalkan*,void*)>
                    (shared::base+0x6C5EA0)(pool,bullet);
            }
            __finally {
                std::memcpy(bytes+0x360,&savedOwner,4);
                std::memcpy(bytes+0x36C,&savedMode,4);
                std::memcpy(bytes+0x1BA,&savedBone,2);
                std::memcpy(bytes+0x190,savedOffset,16);
            }
            return;
        }
        reinterpret_cast<void(__thiscall*)(BehaviorBalkan*,void*)>
            (shared::base+0x6C5EA0)(pool,bullet);
    }
    static bool InstallMuzzleHook()
    {
        static bool installed=false;
        if(installed) return true;
        auto* call=reinterpret_cast<const unsigned char*>(shared::base+0x6E0E45);
        int displacement=0;std::memcpy(&displacement,call+1,4);
        if(call[0]!=0xE8 || shared::base+0x6E0E4A+displacement!=shared::base+0x6C5EA0)
            return false;
        injector::MakeCALL(shared::base+0x6E0E45,PooledShotEffect,true);
        installed=true;return true;
    }
    void EmitPlayerBullet(BehaviorBalkan* pool,Entity* player,Entity* weapon,void* descriptor)
    {
        Entity* previous=emittingOwner;
        Entity* previousWeapon=emittingWeapon;
        const unsigned previousGun=emittingGun;emittingGun=gunIndex;
        emittingOwner=player;emittingWeapon=weapon;
        __try {
            reinterpret_cast<void(__thiscall*)(BehaviorBalkan*,Entity*,void*)>
                (shared::base+0x6E0640)(pool,player,descriptor);
        }
        __finally {emittingOwner=previous;emittingWeapon=previousWeapon;emittingGun=previousGun;}
    }
    float recoilBloom=0.0f;
    float currentSpread=0.015f;
    SmgPolicy::FirearmLoadouts loadouts;
    SmgPolicy::FireControl& fire=loadouts.fire;
    SmgPolicy::AimControl aimControl;
    SmgPolicy::SprayGesture sprayGesture;
    int poseNode=-1;
    unsigned poseTicks=0;
    bool gunPose=false,readyPose=false,poseFrozen=false,raisingPose=false,stanceRaised=false;
    SmgPolicy::ReadyWindow readyWindow;
    NativeGunPose visualPose;
    const char* visualCode=SmgPolicy::IdleMotion;
    float visualFrame=0, locomotionFrame=0;
    const char* locomotionCode=SmgPolicy::IdleMotion;
    float locomotionSpeed=1;
    EntityHandle aimTarget;
    Hw::cVec4 aimPoint;
    bool ikSolved=false;
    bool gunSummoned=false;
    bool focusedAimActive=false;
    SmgPolicy::NinjaJump ninjaJump;
    bool shootingPoseHeld=false;
    unsigned spreadSeed=0x9E3779B9u;
    unsigned animatedCombo=0;
    unsigned preparedDumpCombo=0;
    bool pendingShot=false;
    bool pendingSpray=false;
    bool pendingFocused=false;
    bool groupSpray=false;
    EntityHandle pendingTarget;
    SmgPolicy::Move pendingMove=SmgPolicy::Burst;
    bool reserved=false;
    bool triggerReserved=false;
    unsigned triggerMask=0,triggerOn=0,triggerTrig=0,triggerRep=0;
    unsigned mouseOn=0,mouseTrig=0,mouseRep=0;
    float rightTrigger=0;
    void ReserveTrigger(bool preserveRun=false)
    {
        const unsigned movement=SmgPolicy::Buttons(owner->m_ButtonJump |
            (preserveRun?owner->m_ButtonNinjarun:0));
        triggerMask=SmgPolicy::ControllerFire&~movement;
        triggerOn=g_dbPad.m_On&triggerMask;triggerTrig=g_dbPad.m_Trig&triggerMask;triggerRep=g_dbPad.m_Rep&triggerMask;
        g_dbPad.m_On&=~triggerMask;g_dbPad.m_Trig&=~triggerMask;g_dbPad.m_Rep&=~triggerMask;
        rightTrigger=g_dbPad.m_RightTrigger;
        if(triggerMask&SmgPolicy::ControllerFire) g_dbPad.m_RightTrigger=0;
        mouseOn=g_Mouse.m_On&Hw::MOUSE_BTN_L;mouseTrig=g_Mouse.m_Trig&Hw::MOUSE_BTN_L;mouseRep=g_Mouse.m_Rep&Hw::MOUSE_BTN_L;
        g_Mouse.m_On&=~Hw::MOUSE_BTN_L;g_Mouse.m_Trig&=~Hw::MOUSE_BTN_L;g_Mouse.m_Rep&=~Hw::MOUSE_BTN_L;
        const unsigned attack=SmgPolicy::Buttons(owner->m_ButtonLightAttack)&~movement;
        owner->m_CurrentInput.m_On&=~(attack|triggerMask);
        owner->m_CurrentInput.m_Trig&=~(attack|triggerMask);
        triggerReserved=true;
    }
    unsigned mask=0,padOn=0,padTrig=0,padRep=0,keyOn=0,keyTrig=0,keyRep=0;
    static constexpr unsigned Key=1u<<('C'%32), Constraint=14;
    std::atomic<unsigned> displayRounds{SmgPolicy::Magazine},displayReload{0};
    std::atomic<unsigned> rifleRounds{SmgPolicy::Magazine},vectorRounds{SmgPolicy::Magazine};
    std::atomic<unsigned> displayCharge{0};
    std::atomic<bool> displayReady{false},displayVector{false};
    std::atomic<bool> displayAim{false},displayLock{false},displaySolved{false};
    std::atomic<unsigned> displayStatus{0};
    std::atomic<float> displaySpread{0.015f};
    std::atomic<float> displayScreenX{0.0f}, displayScreenY{0.0f};
    std::atomic<bool> displayHasScreenPos{false};
    struct Impact {EntityHandle target;unsigned cooldown=0;};
    std::vector<Impact> impacts;
    struct HealthBefore {EntityHandle target;int hp;};
    std::vector<HealthBefore> before;
    static bool Stopped() {return Trigger::StpFlags.STP_OBJ!=0;}
    static bool RegisterEffects(unsigned id,Hw::cFmerge* archive)
    {
        __try {return reinterpret_cast<BOOL(__cdecl*)(unsigned,Hw::cFmerge*)>
            (shared::base+0xA00C50)(id,archive)!=FALSE;}
        __except(GunRuntimeLog::Exception(GetExceptionInformation(),"effect registration")) {return false;}
    }
    void Status(unsigned value)
    {
        if(displayStatus.exchange(value)!=value)
            GunRuntimeLog::Write("status=%u gun=%p slot=%d",value,gun.getEntity(),
                g_pPlayerManager?g_pPlayerManager->getSubWeaponEquipped():-1);
    }
    bool ResourcesReady()
    {
        if(Pistol()) {
            if(!PistolAssets::Load(PistolKind())) {Status(10);return false;}
            if(!PistolAssets::LoadAnimations() || !PistolAssets::Motion(PistolPose()) || !PistolAssets::Motion("pistol_grip.bin")) {Status(11);return false;}
        }
        if(!InstallMuzzleHook()) {Status(1);return false;}
        // Fail closed if the verified rifle descriptor differs on this EXE.
        const unsigned char rifleProfile[]={0xC7,0x84,0x24,0x68,0x01,0x00,0x00,0x19,0x00,0x00,0x00};
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x7383E5),rifleProfile,sizeof(rifleProfile))) {Status(1);return false;}
        bool ready=true;
        for(auto& r:resources)
        {
            if(r.id==0x31051 && (!Pistol() || PistolKind()!=PistolPolicy::Socom))continue;
            const auto id=static_cast<eObjID>(r.id);
            const bool optional=r.id==SmgPolicy::GunBank || r.id==SmgPolicy::VectorProfile.model;
            GunRuntimeLog::stage="request assets";
            if(!r.requested)
            {
                r.requested=g_ObjReadManager.requestObject(id,0)!=FALSE;
                if(r.requested) GunRuntimeLog::Write("requested asset=%05X",r.id);
            }
            if(!r.requested || !g_ObjReadManager.isObjectLoaded(id,0))
            {Status(2);ready=false;continue;}
            if(!r.pinned) {g_ObjReadManager.addUseRef(id,0);r.pinned=true;GunRuntimeLog::Write("loaded asset=%05X",r.id);}
            const bool needsBank=r.id==SmgPolicy::MotionBank || r.id==0x31051 || optional;
            if(r.retry) --r.retry;
            if(needsBank && !r.bank && !r.retry && g_ObjReadManager.getDataAtSet(r.archive,id,0))
            {
                if(r.id==SmgPolicy::MotionBank) r.bank=true; // Motion archive needs no effect registration.
                else
                {
                    GunRuntimeLog::stage="register rifle effects";
                    // EffectBullet.bxm names wp0040, not its wp004f streaming
                    // archive. Register and emit under the same native bank.
                    r.bank=r.registered=RegisterEffects(r.id==SmgPolicy::GunBank?SmgPolicy::Gun:r.id,&r.archive);
                    GunRuntimeLog::Write("effect registration asset=%05X bank=%05X result=%d",r.id,SmgPolicy::Gun,int(r.bank));
                    if(!r.bank) r.retry=120;
                }
            }
            // The archive supplies the required grip MOT; effect registration alone is optional.
            ready=(optional || !needsBank || r.bank) && ready;
        }
        if(ready)
        {
            if(!SamStagePolicy::Length(resources[3].archive.getFileNameData("wp0040_0001.mot")))
            {Status(3);return false;}
            for(unsigned i=0;i<16;++i)
            {
                char name[40];std::snprintf(name,sizeof(name),"pl0010_2e%02x.gwm",0x40+i);
                if(!SamStagePolicy::Length(resources[2].archive.getFileNameData(name))) {Status(3);return false;}
            }
            for(const char* code:{SmgPolicy::IdleMotion,SmgPolicy::ReadyMotion,SmgPolicy::RaiseMotion,
                                  SmgPolicy::RecoilMotion,SmgPolicy::ReloadMotion})
            {
                char name[40];std::snprintf(name,sizeof(name),"pl0010_%s.gwm",code);
                if(!SamStagePolicy::Length(resources[2].archive.getFileNameData(name))) {Status(3);return false;}
            }
        }
        return ready;
    }
    void HideGun()
    {
        Entity* entity=gun.getEntity();
        if(owner && entity && owner->getConstraintsEntity(Constraint)==entity) owner->removeConstraint(Constraint);
        if(entity) entity->release();
        gun=EntityHandle{};
    }
    bool Equip()
    {
        if(Entity* entity=gun.getEntity();entity && entity->m_pBehavior)
        {
            if(owner->getConstraintsEntity(Constraint)==entity) return true;
            // Generic attachment can be removed by native weapon transitions.
            if(owner->getConstraintsEntity(Constraint)) {Status(4);return false;}
            Mount(entity);
            return owner->getConstraintsEntity(Constraint)==entity;
        }
        // Do not steal a constraint used by another plugin/native controller.
        if(owner->getConstraintsEntity(Constraint)) {Status(4);return false;}
        GunRuntimeLog::stage="create rifle";
        Entity* entity=nullptr;
        if(Pistol()) {
            // Use the pistol's complete DAT/DTT resource identity. The native
            // virtual loader prepares EntryModelData, GPU buffers and textures.
            // EntityInfo::m_pModelData takes EntryModelData*, not raw WMB bytes.
            EntitySystem::EntityInfo info;
            info.m_pName=Profile().context;info.m_ModelIndex=static_cast<eObjID>(Profile().model);
            info.m_ObjectIndex=static_cast<eObjID>(Profile().model);info.field_14=0;
            GunRuntimeLog::Write("pistol create id=%05X DAT=%p DTT=%p native-model-loader=1",
                Profile().model,PistolAssets::banks[PistolKind()].archive.m_data,
                PistolAssets::banks[PistolKind()].archive.m_dtt);
            entity=g_EntitySystem.createEntity(info);
        } else entity=g_EntitySystem.createEntity(Profile().context,static_cast<eObjID>(Profile().model),nullptr);
        if(!entity || !entity->m_pBehavior) {if(entity) entity->release();Status(5);return false;}
        if(Pistol() && (!entity->m_pBehavior->m_pMeshes || entity->m_pBehavior->m_MeshNum<=0 ||
            !entity->m_pBehavior->getPartsPtr(0))) {
            GunRuntimeLog::Write("pistol render validation failed id=%05X meshes=%p count=%d root=%p",Profile().model,
                entity->m_pBehavior->m_pMeshes,entity->m_pBehavior->m_MeshNum,entity->m_pBehavior->getPartsPtr(0));
            entity->release();Status(12);return false;
        }
        if(Pistol())GunRuntimeLog::Write("pistol rendered model id=%05X meshes=%p count=%d",Profile().model,
            entity->m_pBehavior->m_pMeshes,entity->m_pBehavior->m_MeshNum);
        GunRuntimeLog::Write("created gun=%p behavior=%p context=%s",entity,entity->m_pBehavior,Profile().context);
        gun=entity;entity->m_pBehavior->m_pOwner=owner;
        // The native cyborg equip path also fills cEm0010Weapon::field_8CC
        // (73A73E / 73A9ED). Its update at 7324E7 drops an unbound rifle.
        if(!vectorGun && !Pistol()) static_cast<cEm0010Weapon*>(entity->m_pBehavior)->field_8CC=owner->m_pEntity;
        auto initial=owner->m_pEntity->getTransPos();initial.y+=1.1f;
        entity->setTransPos(initial);entity->setRot(owner->m_Rot);
        GunRuntimeLog::stage="attach rifle";
        Mount(entity);
        entity->m_pBehavior->onDisp();
        GunRuntimeLog::Write("attached constraint=%u actual=%p muzzle=%p",Constraint,
            owner->getConstraintsEntity(Constraint),entity->m_pBehavior->getPartsPtr(Muzzle()));
        if(owner->getConstraintsEntity(Constraint)!=entity) {Status(4);return false;}
        return true;
    }
    void Mount(Entity* entity)
    {
        if(Pistol()) {
            owner->attachObject(Constraint,owner->m_pEntity,entity,13,-1);
            if(auto* mount=owner->getConstraintById(Constraint)) {
                mount->m_vecRotation=Hw::cVec4(0,0,0,0);mount->m_vecOffset=Hw::cVec4(0,0,0,0);
            }
            return;
        }
        if(vectorGun) {
            // Raiden uses the rifle's two-handed pose, whose firing grip is
            // left wrist 9. The enemy right-hand mount leaves the SMG floating
            // beside his hands. wp0020 has no rifle grip MOT, so supply that
            // transform on the constraint instead (once, before visual aim).
            owner->attachObject(Constraint,owner->m_pEntity,entity,9,-1);
            if(auto* mount=owner->getConstraintById(Constraint))
            {
                mount->m_vecRotation=Hw::cVec4(D3DX_PI/2,0,-D3DX_PI/2,0);
                mount->m_vecOffset=Hw::cVec4(-.097f,-.028f,.077f,0);
            }
            return;
        }
        // Native rifle equip (73A3FF..73A438): LEFT wrist 9 and weapon
        // motion 0001. 0000 is the holstered pose, not a hand offset.
        void* grip=resources[3].archive.getFileNameData("wp0040_0001.mot");
        if(!grip) {Status(3);return;}
        auto* weapon=static_cast<cEm0010Weapon*>(entity->m_pBehavior);
        if(weapon->setDirectAnimation(grip,nullptr,0,0,1,0x8000000,0,1)<0) {Status(3);return;}
        weapon->field_8C0=0;weapon->field_8C4=1;weapon->m_Rno0=0;
        owner->attachObject(Constraint,owner->m_pEntity,entity,9,-1);
        // The native weapon MOT supplies all local offsets exactly once.
        if(auto* mount=owner->getConstraintById(Constraint))
        {
            mount->m_vecRotation=Hw::cVec4(0,0,0,0);
            mount->m_vecOffset=Hw::cVec4(0,0,0,0);
        }
    }
    bool OwnsPose() const { return gunPose && poseTicks!=0; }
    void EndPose()
    {
        visualPose.BeginFrame();
        swordParkRequested=false;
        poseTicks=0;poseNode=-1;gunPose=readyPose=poseFrozen=reloadPose=false;
        raisingPose=false;
    }
    void DisplayGunPose()
    {
        if(!owner || !IsEquipped()) return;
        const bool sliding = IsNinjaRunSliding(owner);
        const bool reloading = fire.reload != 0;
        const bool activePose = gunSummoned && Neutral() && !sliding && (!reloading || shootingPoseHeld);
        if(Entity* model=gun.getEntity();model && model->m_pBehavior)
        {
            if(activePose) model->m_pBehavior->onDisp();else model->m_pBehavior->offDisp();
        }
        // The sheath controller borrows the sword attachment for the actual
        // aiming frame. Never hide a sword entity independently of the engine.
    }
    bool IsNinjaRunning(Pl0000* player) const
    {
        if (!player) return false;
        // RT may also be bound to sprint. A fire press alone is not proof
        // that the native movement state has entered Ninja Run.
        return Trigger::StaFlags.STA_NINJARUN != 0;
    }
    const char* BodyClip(Pl0000* player) const
    {
        return player && player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray &&
            player->m_pAnimationSlot->m_Size>0 ? player->m_pAnimationSlot->m_pArray[0].m_pAnimName : nullptr;
    }
    bool IsNinjaRunSliding(Pl0000* player) const
    {
        return player && (Trigger::GameFlags.GAME_SLIDER_NINJARUN_MODE != 0 ||
            SmgPolicy::SlideClip(BodyClip(player)));
    }
    bool UpdateNinjaJump(Pl0000* player)
    {
        const bool jump=((player->m_CurrentInput.m_On|player->m_CurrentInput.m_Trig|
            g_dbPad.m_On|g_dbPad.m_Trig)&SmgPolicy::Buttons(player->m_ButtonJump))!=0 ||
            (GetAsyncKeyState(VK_SPACE)&0x8000)!=0;
        return ninjaJump.Tick(IsNinjaRunning(player),player->isInAir()!=FALSE,jump);
    }
    bool CanShoot(Pl0000* player,bool liveLock) const
    {
        return SmgPolicy::CanFire(Neutral(),IsNinjaRunSliding(player),IsTraversalAction(player),
            ninjaJump.blocked,IsNinjaRunning(player),liveLock);
    }
    bool IsMeleeAction(Pl0000* player) const
    {
        if(!player) return false;
        // Drop the weapon layer as soon as an attack starts, and keep it off
        // through Sam's custom attack states and their native recovery.
        const unsigned attacks=SmgPolicy::Buttons(player->m_ButtonLightAttack|player->m_ButtonHeavyAttack);
        const unsigned nativeAttacks=(focusedAimActive || shootingPoseHeld)?SmgPolicy::Buttons(player->m_ButtonHeavyAttack):attacks;
        if((player->m_CurrentInput.m_On|player->m_CurrentInput.m_Trig)&nativeAttacks) return true;
        if(SamUltimatePolicy::Attack(player->m_Rno0) ||
            (player->m_Rno0>=0x10000Fu && player->m_Rno0<=0x10001Au)) return true;
        // Attack inputs can be released before the native combo recovery ends.
        // Slot zero is the current body animation, so keep the gun layer out
        // until the melee clip has actually yielded control.
        if(player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray &&
            player->m_pAnimationSlot->m_Size>0)
        {
            const char* name=player->m_pAnimationSlot->m_pArray[0].m_pAnimName;
            if(name)
            {
                const size_t length=std::strlen(name);
                for(size_t i=0;i+4<=length;++i)
                {
                    char code[5]={name[i],name[i+1],name[i+2],name[i+3],0};
                    if(SamCombatRouting::IsCombatCode(code)) return true;
                }
            }
        }
        return false;
    }
    bool IsTraversalAction(Pl0000* player) const
    {
        return player && (player->m_Rno0==WallTraversalPolicy::Action ||
            player->m_Rno0==SamUltimatePolicy::Action || SmgPolicy::ParkourClip(BodyClip(player)));
    }
    bool IsLockOnButtonHeld() const
    {
        if (!owner) return false;
        const bool padLock = (g_dbPad.m_On & SmgPolicy::Buttons(owner->m_ButtonSwitchLockOn)) != 0 ||
                             (owner->m_CurrentInput.m_On & SmgPolicy::Buttons(owner->m_ButtonSwitchLockOn)) != 0;
        const bool keyLock = (GetAsyncKeyState('E') & 0x8000) != 0;
        // Camera lock state can remain latched after the player releases the
        // lock button; only live input may engage gun lock-on.
        return padLock || keyLock;
    }
    bool Neutral() const
    {
        if (!owner) return false;
        if (owner->isBladeModeActive()) return false;
        if (Trigger::StaFlags.STA_NINJARUN != 0 && owner->isAlive()) return true;
        return EncounterPolicy::CanThrow(owner->m_Rno0,owner->isAlive()!=FALSE,owner->isInAir()!=FALSE,false,false);
    }
    bool GroundLocomotion() const
    {
        const unsigned interrupt=SmgPolicy::Buttons(owner->m_ButtonLightAttack|owner->m_ButtonHeavyAttack);
        return locomotionCode && !owner->isInAir() && (owner->m_Rno0<=4 || (owner->m_Rno0>=0x100000 && owner->m_Rno0<=0x100002)) &&
            !(owner->m_CurrentInput.m_On&interrupt);
    }
    struct LockTargetInfo
    {
        bool hasLock = false;
        Entity* enemy = nullptr;
        Hw::cVec4 position{};
    };

    static LockTargetInfo GetLockOnTargetInfo(Pl0000* player)
    {
        LockTargetInfo info{};
        if (!player) return info;
        const bool padLock = ((g_dbPad.m_On & SmgPolicy::Buttons(player->m_ButtonSwitchLockOn)) != 0) ||
                             ((player->m_CurrentInput.m_On & SmgPolicy::Buttons(player->m_ButtonSwitchLockOn)) != 0);
        const bool keyLock = (GetAsyncKeyState('E') & 0x8000) != 0;
        const bool inputHeld = padLock || keyLock;
        if (!inputHeld) return info; // Ignore native camera's lingering lock state.
        const bool camActive = g_GameCamera.m_LockonSystem.m_nLockType != 0;

        if (camActive)
        {
            Entity* targetEnt = g_GameCamera.m_LockonSystem.m_TargetEntity.getEntity();
            if (!targetEnt && g_GameCamera.m_LockonSystem.m_pTarget)
                targetEnt = g_GameCamera.m_LockonSystem.m_pTarget->m_pEntity;
            
            if (targetEnt && SamTargets::Enemy(targetEnt) && targetEnt->m_pBehavior &&
                static_cast<BehaviorAppBase*>(targetEnt->m_pBehavior)->m_Hp > 0)
            {
                info.hasLock = true;
                info.enemy = targetEnt;
                const auto& lpos = g_GameCamera.m_LockonSystem.m_vecPosition;
                if (std::isfinite(lpos.x) && std::isfinite(lpos.y) && std::isfinite(lpos.z) &&
                    (lpos.x != 0.0f || lpos.y != 0.0f || lpos.z != 0.0f))
                {
                    info.position = lpos;
                }
                else
                {
                    Hw::cVec4 p = targetEnt->getTransPos();
                    p.y += 0.8f;
                    info.position = p;
                }
                return info;
            }
        }
        if (padLock || keyLock)
        {
            Entity* zTarget = player->field_1370.m_TargetHandle.getEntity();
            if (zTarget && SamTargets::Enemy(zTarget) && zTarget->m_pBehavior &&
                static_cast<BehaviorAppBase*>(zTarget->m_pBehavior)->m_Hp > 0)
            {
                info.hasLock = true;
                info.enemy = zTarget;
                Hw::cVec4 p = zTarget->getTransPos();
                p.y += 0.8f;
                info.position = p;
                return info;
            }
        }
        return info;
    }

    struct VisibleEnemy {Entity* entity;Hw::cVec4 point;float x,score;};
    bool VisiblePoint(const Hw::cVec4& point,float width,float height,const Hw::cVec4& ray,Hw::cVec4& screen) const
    {
        const auto& camera=g_GameCamera.m_Trans;
        const float depth=(point.x-camera.x)*ray.x+(point.y-camera.y)*ray.y+(point.z-camera.z)*ray.z;
        const bool projected=g_GameCamera.worldToScreen(screen,point)!=FALSE;
        return SmgPolicy::OnScreen(projected,depth,screen.x,screen.y,width,height);
    }
    void AimAtUnbounded()
    {
        const auto playerPosition=owner->m_pEntity->getTransPos();
        const int width=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98A90)();
        const int height=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98AA0)();
        aimTarget=EntityHandle{};groupSpray=false;
        aimPoint=playerPosition;aimPoint.x+=std::sin(owner->m_Rot.y)*SmgPolicy::Range;
        aimPoint.z+=std::cos(owner->m_Rot.y)*SmgPolicy::Range;aimPoint.y+=1.3f;
        if(width<=0 || height<=0) return;
        Hw::cVec4 world(0,0,0,1);
        g_GameCamera.screenToWorld(world,Hw::cVec4(width*.5f,height*.5f,.5f,1));
        const auto& c=g_GameCamera.m_Trans;
        D3DXVECTOR3 direction(world.x-c.x,world.y-c.y,world.z-c.z),point;
        if(!GunAimMath::CameraPoint(D3DXVECTOR3(c.x,c.y,c.z),direction,
            D3DXVECTOR3(playerPosition.x,playerPosition.y,playerPosition.z),point)) return;
        aimPoint=Hw::cVec4(point.x,point.y,point.z,1);
        const auto lock=GetLockOnTargetInfo(owner);
        if(lock.hasLock) {aimTarget=lock.enemy;aimPoint=lock.position;return;}
        if(focusedAimActive) return; // held aim follows the camera, without auto targeting

        // Quick taps and juggling prioritize a live native lock, otherwise
        // the nearest live enemy. Free aiming never enters this branch.
        Entity* best=owner->field_26F0.m_TargetHandle.getEntity();
        if(!SamTargets::Enemy(best) || static_cast<BehaviorAppBase*>(best->m_pBehavior)->m_Hp<=0) best=nullptr;
        float nearest=SmgPolicy::Range*SmgPolicy::Range;
        if(!best)
        {
            auto& targets=g_EntitySystem.getEntityList();auto targetIt=targets.begin();
            for(int i=0;i<(std::min)(8192,targets.getSize()) && targetIt!=Hw::cFixedList<Entity*>::iterator{};++i,++targetIt)
            {
                Entity* enemy=*targetIt;
                if(!SamTargets::Enemy(enemy) || static_cast<BehaviorAppBase*>(enemy->m_pBehavior)->m_Hp<=0) continue;
                const auto p=enemy->getTransPos();
                const float dx=p.x-playerPosition.x,dy=p.y-playerPosition.y,dz=p.z-playerPosition.z;
                const float distance=dx*dx+dy*dy+dz*dz;
                if(distance<nearest) {nearest=distance;best=enemy;}
            }
        }
        if(best) {aimTarget=best;aimPoint=best->getTransPos();aimPoint.y+=.8f;}
        return;

    }

    void AimAt()
    {
        // Keep the camera ray as the single source of truth for both the
        // reticle and the projectile. Clamping the player-relative yaw here
        // used to make the visible reticle disagree with the shot direction.
        AimAtUnbounded();
    }
    float NextSpread()
    {
        spreadSeed^=spreadSeed<<13;spreadSeed^=spreadSeed>>17;spreadSeed^=spreadSeed<<5;
        return float(spreadSeed>>8)*(1.0f/16777216.0f);
    }
    bool reloadPose=false;
    void Animate(bool firingRecoil=false,bool ready=false,bool raise=false,bool reload=false)
    {
        if(!Neutral()) return;
        visualCode=reload?SmgPolicy::ReloadMotion:raise?SmgPolicy::RaiseMotion:SmgPolicy::ReadyMotion;
        void* motion=GetVisualMotion(visualCode);
        const unsigned length=SamStagePolicy::Length(motion);if(!length) return;
        visualFrame=0;
        poseTicks=reload?SmgPolicy::Reload:raise?SmgPolicy::RaiseTicks:length;
        gunPose=true;readyPose=ready;reloadPose=reload;raisingPose=raise;
    }
    bool EmpImpact(Pl0000* player,Entity* target) {
        auto& bank=resources[6];
        if(!player || !player->m_pEntity || !target || !bank.bank)return false;
        alignas(16) std::array<unsigned char,0x320> descriptor{};auto* data=descriptor.data();
        using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(data+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(data+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(data);
        PistolPolicy::ConfigureEmp(data);
        *reinterpret_cast<unsigned short*>(data+0x20)=0xA00;
        *reinterpret_cast<Entity**>(data+0x24)=player->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=player->m_pEntity;
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10020000;
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x8800;
        *reinterpret_cast<unsigned*>(data+0x170)=reinterpret_cast<unsigned(__thiscall*)(Pl0000*)>(shared::base+0x5F8B40)(player);
        Hw::cVec4 origin=target->getTransPos();origin.y+=.7f;origin.w=1;
        Hw::cVec4 point=origin,rotation(0,0,0,0);point.y-=.01f;
        reinterpret_cast<void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float)>
            (shared::base+0x16E30)(data,&origin,&point,&rotation,.01f,120.f);
        auto* payload=reinterpret_cast<Behavior*(__cdecl*)(Entity*,void*)>(shared::base+0x6D3BE0)(player->m_pEntity,data);
        GunRuntimeLog::Write("SOCOM EMP impact target=%p payload=%p",target,payload);
        return payload && payload->m_pEntity;
    }
    bool Spawn(Entity* target,SmgPolicy::Move move,bool spray,bool focused)
    {
        Entity* model=gun.getEntity();
        if(!model || !model->m_pBehavior) return false;
        cParts* muzzle=model->m_pBehavior->getPartsPtr(Muzzle());
        if(!muzzle) {Status(6);return false;}
        GunRuntimeLog::stage="construct bullet descriptor";
        alignas(16) std::array<unsigned char,0x320> descriptor{};
        auto* data=descriptor.data();using Init=void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base+0x105D0)(data+0x10);
        reinterpret_cast<Init>(shared::base+0x10710)(data+0x1A0);
        reinterpret_cast<Init>(shared::base+0x1CF30)(data);
        SmgPolicy::Configure(data,move);
        if(Pistol()) {
            const int damage=pistolControl.Damage(PistolKind());
            std::memcpy(data+0x14,&damage,4);
        }
        *reinterpret_cast<Entity**>(data+0x24)=owner->m_pEntity;
        *reinterpret_cast<EntityHandle*>(data+0x28)=owner->m_pEntity;
        *reinterpret_cast<unsigned short*>(data+0x20)=0xA00;
        *reinterpret_cast<unsigned*>(data+0x9C)|=0x10000000;
        *reinterpret_cast<unsigned*>(data+0xA0)|=0x8800;
        auto* category=reinterpret_cast<unsigned*(__thiscall*)(Pl0000*)>(shared::base+0x5F8B60)(owner);
        if(!category) return false;
        *reinterpret_cast<unsigned*>(data+0x170)=*category;
        const float yaw=owner->m_Rot.y;
        Hw::cVec4 origin=muzzle->getPos();origin.w=1;
        const auto& matrix=muzzle->m_LocalMatrix;
        origin.x+=matrix.r[1].x*Profile().muzzleUp+matrix.r[2].x*Profile().muzzleForward;
        origin.y+=matrix.r[1].y*Profile().muzzleUp+matrix.r[2].y*Profile().muzzleForward;
        origin.z+=matrix.r[1].z*Profile().muzzleUp+matrix.r[2].z*Profile().muzzleForward;
        // The current camera selection is authoritative, including while the
        // smoothed running pose is still turning toward the shot direction.
        Hw::cVec4 aim=aimPoint;
        const float dx0=aim.x-origin.x,dy0=aim.y-origin.y,dz0=aim.z-origin.z;
        const float distToAim=(std::max)(2.0f,std::sqrt(dx0*dx0+dy0*dy0+dz0*dz0));
        const auto lockInfo=GetLockOnTargetInfo(owner);
        const bool lockHeld=IsLockOnButtonHeld();
        if(!focused && (!lockInfo.hasLock || lockHeld) && currentSpread>0.001f)
        {
            const float angle=NextSpread()*6.2831853f;
            const float spreadMag=NextSpread()*currentSpread*distToAim;
            const float spX=std::cos(angle)*spreadMag;
            const float spY=std::sin(angle)*spreadMag;
            aim.x+=matrix.r[0].x*spX+matrix.r[1].x*spY;
            aim.y+=matrix.r[0].y*spX+matrix.r[1].y*spY;
            aim.z+=matrix.r[0].z*spX+matrix.r[1].z*spY;
        }
        if(spray)
        {
            if(!target)
            {
                const int width=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98A90)();
                const int height=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98AA0)();
                if(width>0 && height>0)
                {
                    Hw::cVec4 world;g_GameCamera.screenToWorld(world,Hw::cVec4(width*(.2f+.6f*NextSpread()),height*(.35f+.3f*NextSpread()),.5f,1));
                    const auto& c=g_GameCamera.m_Trans;D3DXVECTOR3 point;
                    const auto p=owner->m_pEntity->getTransPos();
                    if(GunAimMath::CameraPoint({c.x,c.y,c.z},{world.x-c.x,world.y-c.y,world.z-c.z},{p.x,p.y,p.z},point))
                        aim=Hw::cVec4(point.x,point.y,point.z,1);
                }
            }
            else
            {
                const float spreadX = (NextSpread() - 0.5f) * 0.8f;
                const float spreadY = (NextSpread() - 0.5f) * 0.4f;
                aim.x += matrix.r[0].x * spreadX + matrix.r[1].x * spreadY;
                aim.y += matrix.r[0].y * spreadX + matrix.r[1].y * spreadY;
                aim.z += matrix.r[0].z * spreadX + matrix.r[1].z * spreadY;
            }
        }
        // The final projectile follows the same camera ray shown by the
        // reticle. A focused shot skips random spread so the reticle is exact.
        const float dx=aim.x-origin.x,dy=aim.y-origin.y,dz=aim.z-origin.z;
        Hw::cVec4 rot(-std::atan2(dy,std::sqrt(dx*dx+dz*dz)),std::atan2(dx,dz),0,0);
        using Trajectory=void(__thiscall*)(void*,Hw::cVec4*,Hw::cVec4*,Hw::cVec4*,float,float);
        reinterpret_cast<Trajectory>(shared::base+0x16E30)(data,&origin,&aim,&rot,SmgPolicy::TrajectoryScale,SmgPolicy::BulletSpeed);
        // Em0010 emits through BehaviorBalkan (6E2BC0 -> 6E0640), whose
        // etb000 geometry renders the native rifle rounds. wpb000 has no model.
        GunRuntimeLog::stage="load native rifle bullet pool";
        if(!g_pPlayerManager) {Status(7);return false;}
        Entity* pool=g_pPlayerManager->getBalkanEntity();
        if(!pool) {g_pPlayerManager->createBalkanEntity();pool=g_pPlayerManager->getBalkanEntity();}
        if(!pool || !pool->m_pBehavior || unsigned(pool->m_ObjId)!=SmgPolicy::BulletPool) {Status(7);return false;}
        auto* balkan=static_cast<BehaviorBalkan*>(pool->m_pBehavior);
        int index=-1;
        for(int i=0;i<250;++i) if(!(balkan->field_1230[i].field_0&1)) {index=i;break;}
        if(index<0) {Status(7);return false;}
        GunRuntimeLog::stage="emit native rifle bullet";
        // The native pool also emits ShotEffData from its owner root (pelvis).
        // Reanchor just that call for this shot; retain collision selector 19,
        // hit effects, damage ownership and the core 26 projectile trail.
        EmitPlayerBullet(balkan,owner->m_pEntity,model,data);
        const auto& emitted=balkan->field_1230[index];
        if(!(emitted.field_0&1) || emitted.field_360.getEntity()!=owner->m_pEntity) {Status(7);return false;}
        bulletPool=pool;
        Status(resources[3].registered?8:9);
        GunRuntimeLog::Write("pooled bullet=%d owner=%p origin=%.3f,%.3f,%.3f yaw=%.3f",index,
            emitted.field_360.getEntity(),origin.x,origin.y,origin.z,yaw);
        // Commit the native projectile independently of cosmetic effects.
        recoilBloom=(std::min)(Profile().bloomMax,recoilBloom+Profile().bloomStep);
        if(Profile().recoilKick>0) visualPose.KickRecoil(Profile().recoilKick);
        fire.Fired();if(fire.reload) readyWindow.Cancel();else readyWindow.Fired();displayRounds=fire.rounds;displayReload=fire.reload;
        if(Pistol())pistolControl.Fired();
        PublishRounds();
        GunRuntimeLog::Write("muzzle v6 native-trail=1 stable-gun-bone=1 player-owner-preserved=%d",int(emitted.field_360.getEntity()==owner->m_pEntity));
        return true;
    }
    void Reserve()
    {
        // Reserve firing only. Composite native button bindings must never
        // consume movement, sprint, jump, or melee input.
        mask=SmgPolicy::ControllerAim;
        padOn=g_dbPad.m_On&mask;padTrig=g_dbPad.m_Trig&mask;padRep=g_dbPad.m_Rep&mask;
        keyOn=g_Keyboard.m_pOn[2]&Key;keyTrig=g_Keyboard.m_pTrig[2]&Key;keyRep=g_Keyboard.m_pRep[2]&Key;
        g_dbPad.m_On&=~mask;g_dbPad.m_Trig&=~mask;g_dbPad.m_Rep&=~mask;
        g_Keyboard.m_pOn[2]&=~Key;g_Keyboard.m_pTrig[2]&=~Key;g_Keyboard.m_pRep[2]&=~Key;
        // The game builds current input from the reserved global sources.
        // Do not mutate its persistent player input snapshot.
        reserved=true;
    }
public:
    inline static unsigned(*pistolStanceCallback)()=nullptr;
    bool IsEquipped() const { return owner!=nullptr && gun.getEntity()!=nullptr && displayReady.load(); }
    bool IsStationary() const
    {
        if(!owner) return true;
        const bool keyboardMove=(GetAsyncKeyState('W')&0x8000) || (GetAsyncKeyState('A')&0x8000) ||
                                (GetAsyncKeyState('S')&0x8000) || (GetAsyncKeyState('D')&0x8000);
        return SmgPolicy::Stationary(owner->m_fInputMagnitudeSquared,
            g_dbPad.m_LeftStick.x,g_dbPad.m_LeftStick.y,keyboardMove,
            owner->m_TransSpeed.x,owner->m_TransSpeed.z);
    }
    void* GetMotion(const char* code)
    {
        if(Pistol())return nullptr; // Handgun poses are visual overlays; native movement owns the body.
        if(!code || !resources[2].bank) return nullptr;
        char name[40]; std::snprintf(name, sizeof(name), "pl0010_%s.mot", code);
        return resources[2].archive.getFileNameData(name);
    }
    void ObserveAnimation(const char* name,int node,float start,float speed)
    {
        if(node!=0) return;
        const char* code=SmgPolicy::Resolve(name);
        locomotionCode=code;locomotionSpeed=speed; // Native graph restarts do not reset the visual stride.
    }
    void* GetVisualMotion(const char* code)
    {
        if(Pistol())return PistolAssets::Motion(PistolPose());
        if(!code || !resources[2].bank) return nullptr;
        char name[40];std::snprintf(name,sizeof(name),"pl0010_%s.gwm",code);
        return resources[2].archive.getFileNameData(name);
    }
    void View(gui::SmgView& out) const
    {
        out.gunIndex=displayGunIndex.load();out.capacity=displayCapacity.load();
        for(unsigned i=0;i<2;++i)out.pistolRounds[i]=pistolRounds[i].load();
        out.rifleRounds=rifleRounds.load();out.vectorRounds=vectorRounds.load();out.vectorGun=displayVector.load();out.rounds=displayRounds.load();out.reload=displayReload.load();out.charge=displayCharge.load();out.ready=displayReady.load();out.status=displayStatus.load();
        out.aiming=displayAim.load();out.locked=displayLock.load();out.solved=displaySolved.load();
        out.spread=displaySpread.load();out.screenX=displayScreenX.load();out.screenY=displayScreenY.load();out.hasScreenPos=displayHasScreenPos.load();
    }
    void RestoreInputs()
    {
        if(triggerReserved)
        {
            g_dbPad.m_On=(g_dbPad.m_On&~triggerMask)|triggerOn;g_dbPad.m_Trig=(g_dbPad.m_Trig&~triggerMask)|triggerTrig;
            g_dbPad.m_Rep=(g_dbPad.m_Rep&~triggerMask)|triggerRep;g_dbPad.m_RightTrigger=rightTrigger;
            g_Mouse.m_On=(g_Mouse.m_On&~Hw::MOUSE_BTN_L)|mouseOn;g_Mouse.m_Trig=(g_Mouse.m_Trig&~Hw::MOUSE_BTN_L)|mouseTrig;
            g_Mouse.m_Rep=(g_Mouse.m_Rep&~Hw::MOUSE_BTN_L)|mouseRep;triggerReserved=false;
        }
        if(!reserved) return;
        g_dbPad.m_On=(g_dbPad.m_On&~mask)|padOn;g_dbPad.m_Trig=(g_dbPad.m_Trig&~mask)|padTrig;g_dbPad.m_Rep=(g_dbPad.m_Rep&~mask)|padRep;
        g_Keyboard.m_pOn[2]=(g_Keyboard.m_pOn[2]&~Key)|keyOn;g_Keyboard.m_pTrig[2]=(g_Keyboard.m_pTrig[2]&~Key)|keyTrig;
        g_Keyboard.m_pRep[2]=(g_Keyboard.m_pRep[2]&~Key)|keyRep;reserved=false;
    }
    void Deactivate()
    {
        RestoreInputs();
        fire.Cancel();
        pistolControl.Cancel();
        aimControl.Cancel();shootingPoseHeld=false;
        sprayGesture.Reset();
        gunSummoned=false;
        displayAim=false;displayLock=false;displaySolved=false;
        focusedAimActive=false;
        ninjaJump.Reset();
        readyWindow.Cancel();
        pendingShot=false;
        pendingTarget=EntityHandle{};
        dumpTarget=EntityHandle{};
        swordParkRequested=false;
        displayReady=false; // Cancellation must bypass gun motion remapping.
        EndPose();
        visualPose.Reset();
        stanceRaised=false;
        HideGun();
        displayReady=false;
        displayCharge=0;
    }
    void Forget(Pl0000* player)
    {
        if(owner!=player) return;
        Deactivate();bulletPool=EntityHandle{};impacts.clear();before.clear();owner=nullptr;loadouts.Reset();recoilBloom=0;rifleRounds=vectorRounds=SmgPolicy::Magazine;
        pistolSleep.Reset();seenPistolHits.clear();pistolRounds[0]=15;pistolRounds[1]=12;
        animatedCombo=preparedDumpCombo=0;
    }
    // Only after native player teardown, as for Sam/knife archive references.
    void SceneReleased()
    {
        for(auto& r:resources)
        {
            if(r.registered) reinterpret_cast<void(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00D60)
                (r.id==SmgPolicy::GunBank?SmgPolicy::Gun:r.id,&r.archive);
            if(r.pinned) g_ObjReadManager.removeReference(static_cast<eObjID>(r.id),0);
            if(r.requested) g_ObjReadManager.removeRequest(static_cast<eObjID>(r.id),0);
            r.bank=r.registered=r.pinned=r.requested=false;r.retry=0;
        }
    }
    void Tick(Pl0000* player,bool selected,unsigned nextGun=0)
    {
        visualPose.BeginFrame();
        if(selected && nextGun<NativeSmgMenuPolicy::GunCount && nextGun!=gunIndex) {
            Deactivate();
            loadouts.Select(nextGun);
            gunIndex=nextGun;vectorGun=gunIndex==1;displayVector=vectorGun;
            displayGunIndex=gunIndex;displayCapacity=fire.capacity;
            fire.Cancel();recoilBloom=0;currentSpread=0.015f;
            displayRounds=fire.rounds;displayReload=fire.reload;
            PublishRounds();
        }
        if(GunRuntimeLog::selected!=selected)
        {GunRuntimeLog::selected=selected;GunRuntimeLog::Write("selected=%d build=gun-runtime-20 player=%p",int(selected),player);}
        GunRuntimeLog::stage="gun eligibility";
        pendingShot=false;pendingTarget=EntityHandle{};
        pendingFocused=false;
        if(owner!=player) {if(owner) Forget(owner);owner=player;}
        before.clear();
        if(bulletPool.getEntity() || selected)
        {
            auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
            for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
            {
                Entity* target=*it;if(!SamTargets::Enemy(target)) continue;
                HealthBefore entry;entry.target=target;entry.hp=static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp;
                before.push_back(entry);
            }
        }
        if(!selected) {Deactivate();return;}
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool story=EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        const bool eligible=player && player->m_pEntity && SamNativeRuntime::Get().Owns(player) &&
            foreground==GetCurrentProcessId() && !gui::IsMenuVisible() && !g_StaFlags.STA_PAUSE &&
            EncounterPolicy::CanThrow(player->m_Rno0,player->isAlive()!=FALSE,player->isInAir()!=FALSE,story,player->isBladeModeActive()!=FALSE);
        static bool wasEligible=false;
        if(eligible!=wasEligible)
        {
            wasEligible=eligible;
            GunRuntimeLog::Write("eligible=%d owned=%d focus=%d menu=%d pause=%d story=%d state=%X",
                int(eligible),int(player && SamNativeRuntime::Get().Owns(player)),int(foreground==GetCurrentProcessId()),
                int(gui::IsMenuVisible()),int(g_StaFlags.STA_PAUSE),int(story),player?player->m_Rno0:0);
        }
        if(!eligible) {Deactivate();return;}
        Reserve();
        if(Stopped()) return;
        const bool sliding = IsNinjaRunSliding(player);
        UpdateNinjaJump(player);
        const bool aimButton = ((GetAsyncKeyState('C')&0x8000)!=0) ||
            SmgPolicy::ControllerAimHeld(g_dbPad.m_On|padOn);
        float flickX=g_dbPad.m_LeftStick.x,flickY=g_dbPad.m_LeftStick.y;
        const float keyX=float((GetAsyncKeyState('D')&0x8000)!=0)-float((GetAsyncKeyState('A')&0x8000)!=0);
        const float keyY=float((GetAsyncKeyState('W')&0x8000)!=0)-float((GetAsyncKeyState('S')&0x8000)!=0);
        if(keyX || keyY) {flickX=keyX;flickY=keyY;}
        const bool lockHeld=GetLockOnTargetInfo(player).hasLock;
        const bool canShoot=CanShoot(player,lockHeld);
        const bool mouseHeld=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
        const bool trigger=mouseHeld || SmgPolicy::ControllerFireHeld(g_dbPad.m_On|padOn,g_dbPad.m_RightTrigger);
        const bool held=Pistol()?false:aimControl.Tick(aimButton,trigger,canShoot,sliding,lockHeld);
        const bool focusedAim=Pistol()?aimButton:aimControl.aiming;
        if(Pistol())pistolControl.Tick(aimButton,trigger,canShoot && !sliding,PistolKind()==PistolPolicy::Socom,!fire.reload);
        focusedAimActive=focusedAim || (Pistol() && pistolControl.pending);
        shootingPoseHeld=Pistol()?aimButton:held;
        if(canShoot && (focusedAim || held)) ReserveTrigger(lockHeld && IsNinjaRunning(player));
        const bool wide=sprayGesture.Tick(flickX,flickY,held) && !lockHeld && !focusedAim;

        recoilBloom=(std::max)(0.0f,recoilBloom-0.0025f);
        const bool stationary=IsStationary();
        const bool ninjaRunning=IsNinjaRunning(player);
        float baseSpr=0.015f;
        if(!stationary) baseSpr+=0.025f;
        if(ninjaRunning) baseSpr+=0.035f;
        if(wide) baseSpr+=0.060f;
        if(lockHeld && !wide) baseSpr=0.005f;
        currentSpread=baseSpr+recoilBloom;
        displaySpread=currentSpread;

        const auto move=lockHeld ? SmgPolicy::Launcher : (owner->isInAir()?SmgPolicy::Aerial:wide?SmgPolicy::Sweep:SmgPolicy::Burst);
        fire.Tick(held,canShoot,move,!Pistol() && aimControl.tapped);
        if(Pistol())fire.queued=pistolControl.pending && !fire.reload?1u:0u;
        fire.RejectUnfunded();
        const bool reloading = fire.reload != 0;
        if(reloading) readyWindow.Cancel();
        const bool firingOrReady = !reloading && (held || fire.queued != 0 || fire.delay != 0 || readyWindow.remaining != 0);
        const bool melee=IsMeleeAction(player);
        const bool dropForReload=reloading && !shootingPoseHeld;
        gunSummoned = canShoot && !sliding && !melee && !dropForReload &&
            ((firingOrReady || focusedAim) && !fire.charge.requireRelease);
        if(shootingPoseHeld && canShoot && !sliding && !melee && !fire.charge.requireRelease) gunSummoned=true;
        displayCharge=Pistol()?pistolControl.hold.frames:0;
        displayRounds=fire.rounds;displayReload=fire.reload;
        PublishRounds();
        readyWindow.Tick(true,canShoot && !reloading);
        if(!gunSummoned || melee || dropForReload)
        {
            visualPose.Reset();
            gunPose=readyPose=raisingPose=reloadPose=false;poseTicks=0;stanceRaised=false;
        }
        displayReady=ResourcesReady() && Equip();
        if(displayReady && displayStatus!=6 && displayStatus!=7) Status(resources[3].registered?8:9);
        if(!displayReady) return;
        static DWORD lastSnapshot=0;
        const DWORD now=GetTickCount();
        if(now-lastSnapshot>=2000)
        {
            lastSnapshot=now;
            auto* model=gun.getEntity();auto* behavior=model->m_pBehavior;
            auto* root=behavior->getPartsPtr(Muzzle());
            GunRuntimeLog::Write("gun snapshot entity=%p model=%p flags=%08X binding=%p constraint=%p root=%p rounds=%u queued=%u delay=%u",
                model,behavior->m_pModel,behavior->m_ObjectFlags,
                (vectorGun || Pistol())?behavior->m_pOwner->m_pEntity:static_cast<cEm0010Weapon*>(behavior)->field_8CC.getEntity(),owner->getConstraintsEntity(Constraint),
                root,fire.rounds,fire.queued,fire.delay);
        }
        GunRuntimeLog::stage="gun pose and firing";
        if(!gunSummoned || melee || dropForReload) return;
        // Fire enters the ready grip immediately from any permitted action.
        // Reload removes IK and gun holding poses so native animations play cleanly.
        visualCode=SmgPolicy::ReadyMotion;
        visualFrame=0;
        gunPose=readyPose=true;raisingPose=reloadPose=false;poseTicks=1;
        AimAt();
        if(!fire.Ready()) return;
        pendingTarget=aimTarget;
        pendingMove=Pistol()?SmgPolicy::Burst:(lockHeld || fire.move==SmgPolicy::Launcher) ? SmgPolicy::Launcher :
            (owner->isInAir()?SmgPolicy::Aerial:SmgPolicy::RoundMove(fire.move,fire.ordinal));
        pendingSpray=!Pistol() && !focusedAim && SmgPolicy::Spray(fire.move,wide);pendingShot=true;
        pendingFocused=focusedAim || lockHeld || aimControl.tapped || (Pistol() && pistolControl.pending);
    }
    inline static bool(*lowPoseCallback)(Pl0000*)=nullptr;
    bool UsesReadyPose() const {return IsEquipped() && swordParkRequested;}
    void RestoreVisualPose() {visualPose.BeginFrame(gunSummoned && IsEquipped());}
    void PostTick(Pl0000* player)
    {
        swordParkRequested=false;
        if(owner!=player || !player || !player->m_pEntity || Stopped()) return;
        const bool ninjaRunning=IsNinjaRunning(player);
        UpdateNinjaJump(player);
        const bool canShoot=CanShoot(player,GetLockOnTargetInfo(player).hasLock);
        if(!canShoot)
        {
            pendingShot=false;fire.Cancel();pistolControl.Cancel();aimControl.Cancel();readyWindow.Cancel();
            focusedAimActive=shootingPoseHeld=gunSummoned=false;
        }
        const bool sliding=IsNinjaRunSliding(player);
        const bool melee=IsMeleeAction(player);
        const bool traversal=IsTraversalAction(player);
        const bool reloading=fire.reload!=0;
        const bool dropForReload=reloading && !shootingPoseHeld;
        const bool airborne=player->isInAir()!=FALSE;
        // Overlay only the upper body during grounded locked-on Ninja Run.
        // Traversal keeps the complete native pose and cancels queued shots.
        const bool overlay=canShoot && !melee && !dropForReload &&
            SmgPolicy::GunOverlay(sliding,traversal,airborne,GroundLocomotion(),ninjaRunning);
        swordParkRequested=IsEquipped() && gunSummoned && Neutral() && overlay && !reloadPose && !raisingPose;
        if(!overlay || melee || sliding || traversal || dropForReload)
        {
            visualPose.Reset();
            gunPose=readyPose=raisingPose=reloadPose=false;poseTicks=0;stanceRaised=false;
        }
        const bool low=IsEquipped() && gunSummoned && Neutral() && overlay && lowPoseCallback && lowPoseCallback(player);
        if(IsEquipped() && gunSummoned && Neutral() && overlay)
        {
            AimAt();
            visualPose.Prepare(player,GetVisualMotion(SmgPolicy::ReadyMotion),aimPoint,0.12f);
            const bool stationary=IsStationary();
            const float moveSpeed=std::hypot(player->m_TransSpeed.x,player->m_TransSpeed.z);
            const bool sprint=(player->m_CurrentInput.m_On & SmgPolicy::Buttons(player->m_ButtonNinjarun))!=0 || moveSpeed>.10f;
            // Keep the native pelvis and legs even at zero speed: an idle
            // override can freeze the root on the first frame of movement.
            // A stable two-handed ready sample avoids replaying donor scan,
            // reload and raise motion on every burst or at loop boundaries.
            const float readyFrame=Pistol()?float(pistolStanceCallback && pistolStanceCallback()==2?
                PistolHandling::ProneFrame:PistolHandling::StandingFrame):0.0f;
            if(!low || Pistol())visualPose.Apply(player,GetVisualMotion(SmgPolicy::ReadyMotion),readyFrame,true,low);
            if(!stationary && !airborne) locomotionFrame+=SmgPolicy::StrideRate(moveSpeed,sprint);
        }
        const bool previouslySolved=ikSolved;ikSolved=false;
        if(IsEquipped() && gunSummoned && Neutral() && overlay && !reloadPose && !raisingPose)
        {
            if(Entity* model=gun.getEntity();model && model->m_pBehavior)
            {
                GunRuntimeLog::stage="player gun upper-body aim";
                AimAt(); // Camera/lock position after the native frame update.
                const auto lockInfo = GetLockOnTargetInfo(player);
                D3DXMATRIX pistolGrip;const D3DXMATRIX* grip=nullptr;
                if(Pistol()) {if(void* data=PistolAssets::Motion("pistol_grip.bin")) {std::memcpy(&pistolGrip,data,64);grip=&pistolGrip;}}
                const bool solved=visualPose.Aim(player,model->m_pBehavior,aimPoint,
                    D3DX_PI*.36f,lockInfo.hasLock?0.35f:0.075f,vectorGun,grip,Profile().muzzleUp,Profile().muzzleForward);
                ikSolved=solved;
                if(solved!=previouslySolved)
                {GunRuntimeLog::Write("upper-body aim solved=%d target=%p aim=(%.3f,%.3f,%.3f)",int(solved),aimTarget.getEntity(),aimPoint.x,aimPoint.y,aimPoint.z);}
                static DWORD lastGripLog=0;
                const DWORD gripNow=GetTickCount();
                if(solved && gripNow-lastGripLog>=2000)
                {
                    lastGripLog=gripNow;
                    auto* root=model->m_pBehavior->getPartsPtr(0);
                    auto* wrist=player->getPartsPtr(Pistol()?13:9);
                    if(root && wrist)
                    {
                        const auto& p=root->getPos();const auto& w=wrist->getPos();
                        const auto& f=root->m_LocalMatrix.r[2];
                        GunRuntimeLog::Write("post-aim grip wrist=(%.3f,%.3f,%.3f) gun=(%.3f,%.3f,%.3f) barrel=(%.3f,%.3f,%.3f) aim=(%.3f,%.3f,%.3f)",
                            w.x,w.y,w.z,p.x,p.y,p.z,f.x,f.y,f.z,aimPoint.x,aimPoint.y,aimPoint.z);
                    }
                }
            }
        }
        DisplayGunPose();
        displayAim=IsEquipped() && focusedAimActive && gunSummoned && Neutral() &&
            !g_StaFlags.STA_PAUSE && !melee;
        displayLock=IsLockOnButtonHeld() && aimTarget.getEntity()!=nullptr;displaySolved=ikSolved;

        const int width=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98A90)();
        const int height=reinterpret_cast<int(__cdecl*)()>(shared::base+0xB98AA0)();
        Hw::cVec4 screen(0,0,0,0);
        if(width>0 && height>0 && g_GameCamera.worldToScreen(screen,aimPoint)!=FALSE && screen.w>0.0f && screen.z>=0.0f)
        {
            displayScreenX=screen.x;
            displayScreenY=screen.y;
            displayHasScreenPos=true;
        }
        else
        {
            displayHasScreenPos=false;
        }

        const bool react=player->isAlive() && !g_StaFlags.STA_PAUSE && !EncounterPolicy::Scripted(g_StaFlags.STA_EVENT,g_StaFlags.STA_QTE,g_StaFlags.STA_CODEC,g_StaFlags.STA_SOFT_EVENT);
        // Native TickGame updates the hand constraint and weapon muzzle bones.
        // Emit afterward so the bullet/flash use the gun's rendered position,
        // including during a charge-to-shoot animation transition.
        if(pendingShot)
        {
            pendingShot=false;
            if(react && canShoot && !melee && g_pPlayerManager && g_pPlayerManager->getSubWeaponEquipped()==NativeSmgMenuPolicy::SlotFor(gunIndex) &&
                EncounterPolicy::CanThrow(player->m_Rno0,true,player->isInAir()!=FALSE,false,player->isBladeModeActive()!=FALSE))
            {
                Spawn(aimTarget.getEntity(),pendingMove,pendingSpray || groupSpray,pendingFocused);
            }
            else fire.Cancel();
        }
        ++impactFrame;
        for(auto& impact:impacts) if(impact.cooldown) --impact.cooldown;
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        for(int i=0;i<(std::min)(8192,list.getSize()) && it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
        {
            Entity* target=*it;
            if(!target || !target->m_pBehavior || (!SamTargets::Enemy(target) && !SamElectricPolicy::Ordinary(unsigned(target->m_ObjId))))continue;
            auto* actor=static_cast<BehaviorAppBase*>(target->m_pBehavior);
            const auto& hits=actor->m_AttackHits;if(!hits.m_vector || hits.m_size>hits.m_capacity || hits.m_size>128) continue;
            for(size_t h=0;h<hits.m_size;++h)
            {
                const auto& hit=hits.m_vector[h];const auto& data=hit.m_HitData;
                const bool pistolHit=data.field_0==SmgPolicy::Attack && PistolPolicy::Round(data.field_4) && data.field_18.getEntity()==player->m_pEntity;
                if(!pistolHit && !SmgPolicy::OwnedHit(data.field_0,data.field_4,data.field_18.getEntity()==player->m_pEntity)) continue;
                bool identified=bulletPool.getEntity() &&
                    (!std::memcmp(&hit.field_120,&bulletPool,sizeof(EntityHandle)) ||
                     !std::memcmp(&hit.field_124,&bulletPool,sizeof(EntityHandle)));
                bool lostHp=false;
                for(const auto& snapshot:before) if(snapshot.target.getEntity()==target) {lostHp=actor->m_Hp<snapshot.hp;break;}
                // Pistol rounds have a private attack/damage signature and a
                // verified player owner. Native zero-damage contacts need not
                // report the pool entity in both auxiliary attacker handles.
                if(!react || (!pistolHit && (!identified || !lostHp))) continue;
                if(pistolHit && !NewPistolHit(target,hit))continue;
                auto found=std::find_if(impacts.begin(),impacts.end(),[target](const auto& v){return v.target.getEntity()==target;});
                if(found==impacts.end()) {Impact v;v.target=target;impacts.push_back(v);found=impacts.end()-1;}
                if(found->cooldown) continue;
                if(pistolHit) {
                    if(data.field_4==18 || data.field_4==30)EmpImpact(player,target);
                    if(actor->m_Hp<=0 || !SamElectricPolicy::Ordinary(unsigned(target->m_ObjId))) {found->cooldown=15;continue;}
                    const auto p=player->m_pEntity->getTransPos(),v=target->getTransPos();
                    const float dx=v.x-p.x,dz=v.z-p.z,length=std::hypot(dx,dz);
                    if(length>.001f) {
                        const float kick=PistolPolicy::Kick(data.field_4);
                        actor->m_TransSpeed.x+=dx/length*kick;actor->m_TransSpeed.z+=dz/length*kick;
                    }
                    if(data.field_4==0)pistolSleep.Hit(target);
                    found->cooldown=data.field_4==0?18:15;continue;
                }
                if(actor->m_Hp<=0 || !SamElectricPolicy::Ordinary(unsigned(target->m_ObjId)))continue;
                if(data.field_0==SmgPolicy::LaunchAttack && data.field_4==6)
                {
                    actor->m_TransSpeed.y=(std::max)(actor->m_TransSpeed.y,0.48f);
                    auto pos=target->getTransPos();pos.y+=0.25f;target->setTransPos(pos);
                    found->cooldown=30;
                }
                else if(!actor->isOnGround())
                {
                    actor->m_TransSpeed.y=SmgPolicy::JuggleSpeed(actor->m_TransSpeed.y);found->cooldown=4;
                    // Reward actual airborne hits without suspending misses.
                    if(player->isInAir()) player->m_TransSpeed.y=SmgPolicy::AirComboSpeed(player->m_TransSpeed.y);
                }
            }
        }
        impacts.erase(std::remove_if(impacts.begin(),impacts.end(),[](const auto& v){return !v.target.getEntity() || !v.cooldown;}),impacts.end());
        pistolSleep.Tick(player,react && !Stopped());
        seenPistolHits.erase(std::remove_if(seenPistolHits.begin(),seenPistolHits.end(),[&](const auto& v){return !v.target.getEntity() || impactFrame-v.frame>120;}),seenPistolHits.end());
    }
};
