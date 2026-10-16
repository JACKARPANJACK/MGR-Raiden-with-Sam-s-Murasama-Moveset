#pragma once
#include "DualBladePolicy.h"
#include "SamArchiveLookup.h"
#include "SamConfig.h"
#include "WeaponAttachment.h"
#include "PistolAssets.h"
#include <EntitySystem.h>
#include <Pl0000.h>
#include <cObjReadManager.h>
#include <PlayerManagerImplement.h>
#include <vector>
#include <atomic>
#include <cstdio>
#include "../SafeHook/SafeHook.h"

class DualBladeWeapon
{
    Pl0000* owner=nullptr;
    EntityHandle blade{};
    bool loaded=false;
    int savedSwordUpdate=1, savedSwordAttach=0, savedHidden=0;
    int savedCustomBone=0, savedAttachmentBone=0;
    float savedCustomOpacity=0.0f;
    unsigned diagnosticTicks=0;
    std::atomic<const char*> status{"wp2050 not selected"};
    std::vector<char> data; // retained while any animation/model node references it
    Hw::cFmerge archive, weaponArchive;
    void Report(const char* message)
    {
        if(status.exchange(message)==message) return;
        std::string path=SamConfig::GetIniPath();path.resize(path.find_last_of("\\/")+1);
        path+="scripts\\DualBladeRuntime.log";
        FILE* file=nullptr;if(!fopen_s(&file,path.c_str(),"a") && file) {
            fprintf(file,"[%lu] %s\n",GetTickCount(),message);fclose(file);
        }
    }
    bool Load()
    {
        if(loaded) return true;
        std::string path=SamConfig::GetIniPath();path.resize(path.find_last_of("\\/")+1);
        path+="GameData\\wp\\wp2050.dat";
        FILE* file=nullptr;
        if(fopen_s(&file,path.c_str(),"rb") || !file) {Report("wp2050 ERROR: GameData/wp/wp2050.dat missing");return false;}
        fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);
        if(size<32 || size>32*1024*1024) {fclose(file);Report("wp2050 ERROR: invalid archive size");return false;}
        data.resize(size);bool ok=fread(data.data(),1,size,file)==static_cast<size_t>(size);fclose(file);
        if(!ok || std::memcmp(data.data(),"DAT\0",4)) {Report("wp2050 ERROR: invalid DAT");return false;}
        archive.setData(data.data());
        if(!archive.getFileNameData("wp2050.wmb")) {Report("wp2050 ERROR: small katana model missing from mod archive");return false;}
        auto* modelBank=static_cast<char*>(archive.getFileNameData("wp2050_weapon.dat"));
        if(!modelBank) {Report("wp2050 ERROR: outdated archive, weapon bank missing");return false;}
        weaponArchive.setData(modelBank);
        if(!weaponArchive.getFileNameData("wp2050.wmb")) {Report("wp2050 ERROR: small katana model missing");return false;}
        for(auto code:DualBladePolicy::Clips) if(!GetClip(code).motion || !GetClip(code).sequence) {
            Report("wp2050 ERROR: animation pair missing");return false;
        }
        loaded=true;Report("wp2050 resource loaded: wp2050.wmb available in archive and native weapon stream bank");return true;
    }
public:
    static DualBladeWeapon& Get() {static DualBladeWeapon value;return value;}
    const char* Status() const {return status.load();}
    bool Active(Behavior* actor) const
    {
        if(!owner || actor!=owner || !blade.getEntity()) return false;
        if(g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()!=0) return false;
        if(owner->getConstraintsEntity(DualBladePolicy::Constraint)!=blade.getEntity()) return false;
        if(owner->m_CustomWeaponHandle.getEntity()!=blade.getEntity()) return false;
        return true;
    }
    Entity* BladeEntity() const { return blade.getEntity(); }
    bool Prepare() {return Load();}
    bool Supply(Hw::cFmerge& output) {if(!Load()) return false;output=weaponArchive;return true;}
    SamArchiveLookup::Clip GetClip(const char* code)
    {
        const char* prefix=DualBladePolicy::IsSundownerClip(code)?"pl0010":"wp2050";
        char name[64]{};std::snprintf(name,sizeof(name),"%s_%s.mot",prefix,code);
        void* motion=archive.getFileNameData(name);
        std::snprintf(name,sizeof(name),"%s_%s_0_seq.bxm",prefix,code);
        return {motion,archive.getFileNameData(name),SamArchiveLookup::Source::Playable};
    }
    Entity* Create(Pl0000* player)
    {
        if(!player || !player->m_pEntity || !Load()) return nullptr;
        auto* sword=player->m_SwordHandle.getEntity();
        if(!sword || !sword->m_pBehavior || !sword->m_pBehavior->m_pMeshes || sword->m_pBehavior->m_MeshNum<=0) {
            Report("wp2050 ERROR: player katana render model unavailable");return nullptr;
        }
        // Use the same cPl0000Weapon behavior as Raiden's native custom
        // weapons, with the streamed small-blade model prepared by the engine.
        EntitySystem::EntityInfo info;
        info.m_pName="Pl0010_SUBWEP1";
        info.m_ModelIndex=static_cast<eObjID>(DualBladePolicy::Object);
        info.m_ObjectIndex=static_cast<eObjID>(DualBladePolicy::WeaponObject);
        info.field_14=0;
        Entity* entity=g_EntitySystem.createEntity(info);
        if(!entity || !entity->m_pBehavior) {
            if(entity) entity->release();Report("wp2050 ERROR: player custom-weapon creation failed");return nullptr;
        }
        auto* behavior=entity->m_pBehavior;
        if(!behavior->m_pMeshes || behavior->m_MeshNum<=0) {
            entity->release();Report("wp2050 ERROR: small blade has no render meshes");return nullptr;
        }
        Report("wp2050 created: cPl0000Weapon with native small-blade render model");return entity;
    }
    bool Commit(Pl0000* player,Entity* entity)
    {
        savedSwordUpdate=player->field_13F8;savedSwordAttach=player->field_13F4;savedHidden=player->m_bSwordHidden;
        savedCustomBone=player->m_CustomWeaponBone;savedAttachmentBone=player->field_1420;
        savedCustomOpacity=player->field_1428;
        // Slot 5 is the game-managed custom weapon attachment. Binding the
        // actual wp2050 entity and marker also lets native weapon maintenance
        // keep it attached while Raiden's independent sword stays in slot 4.
        if(!WeaponAttachment::Bind(player,entity,DualBladePolicy::HandBone,DualBladePolicy::GripBone)) {
            player->removeConstraint(DualBladePolicy::Constraint);
            player->m_CustomWeaponHandle=static_cast<Entity*>(nullptr);
            player->m_CustomWeaponBone=savedCustomBone;player->field_1420=savedAttachmentBone;
            entity->release();Report("wp2050 ERROR: native off-hand attachment rejected");return false;
        }
        owner=player;blade=entity;diagnosticTicks=0;
        auto* b=entity->m_pBehavior;
        // Keep the weapon on its native neutral pose. The grip bone is
        // constrained to Raiden's hand directly; playing a donor weapon MOT
        // here creates an unrelated looping motion during idle.
        ShowBlade(b);
        DrawKatana(true);Report("wp2050 attached and verified: player katana at 700 + small katana at 701 in native slot 5");
        return true;
    }
    void ShowBlade(Behavior* behavior)
    {
        if(!owner || !behavior) return;
        // Native RVA 7E93E9-7E9416 copies field_1428 to cMesh::m_Color.a.
        // Sword combat has no custom-weapon draw flag, so native maintenance
        // fades this to zero. onDisp and camera alpha cannot undo that fade.
        owner->field_1428=1.0f;
        behavior->m_CamAlphaRate=1.0f;
        behavior->m_CurrentCamAlphaRate=1.0f;
        for(int i=0;i<behavior->m_MeshNum;++i) {
            behavior->m_pMeshes[i].m_MeshFlags |= 1u;
            behavior->m_pMeshes[i].m_Color.a=1.0f;
            behavior->m_pMeshes[i].m_ColorScale.a=1.0f;
        }
        behavior->onDisp();
    }
    void DrawKatana(bool forceUnsheathe=false)
    {
        if(!owner) return;
        Entity* sword=owner->m_pBladeEntity?owner->m_pBladeEntity:owner->m_SwordHandle.getEntity();if(!sword) return;
        auto* c=owner->getConstraintById(4);
        const bool needsAttach=!c || c->m_MainEntity.getEntity()!=owner->m_pEntity ||
            c->m_ObjectEntityHandle.getEntity()!=sword || c->m_nBone!=0x700;
        owner->m_bSwordHidden=0;owner->field_13F4=1;owner->field_13F8=0;
        if(needsAttach) {
            if(owner->m_pSheathEntity && owner->m_pSheathEntity->m_pBehavior)
                owner->m_pSheathEntity->m_pBehavior->removeConstraint(4);
            owner->removeConstraint(4);
            owner->attachObject(4,owner->m_pEntity,sword,0x700,0);
            owner->setConstraintsBone(4,0x700,0);
        }
        reinterpret_cast<void(__thiscall*)(Pl0000*, BOOL)>(shared::base+0x77E210)(owner,FALSE);
        if(sword->m_pBehavior) {
            Pl0012* swordInstance=reinterpret_cast<Pl0012*>(sword->m_pBehavior);
            ((char* (__thiscall*)(int))(shared::base+0x811BE0))(reinterpret_cast<int>(swordInstance));
            sword->m_pBehavior->onDisp();
        }
    }
    void Release(Pl0000* player)
    {
        if(owner && owner==player) {
            if(auto* entity=blade.getEntity()) {
                if(owner->getConstraintsEntity(DualBladePolicy::Constraint)==entity)
                    owner->removeConstraint(DualBladePolicy::Constraint);
                if(owner->m_CustomWeaponHandle.getEntity()==entity) owner->m_CustomWeaponHandle=static_cast<Entity*>(nullptr);
                entity->release();
            }
            owner->field_13F8=savedSwordUpdate;owner->field_13F4=savedSwordAttach;owner->m_bSwordHidden=savedHidden;
            if(owner->m_CustomWeaponBone==DualBladePolicy::HandBone) owner->m_CustomWeaponBone=savedCustomBone;
            if(owner->field_1420==DualBladePolicy::HandBone) owner->field_1420=savedAttachmentBone;
            owner->field_1428=savedCustomOpacity;
            blade=static_cast<Entity*>(nullptr);owner=nullptr;Report("wp2050 released");
        }
    }
    void Tick(Pl0000* player)
    {
        if(owner && (owner!=player || !player || !player->isAlive() ||
           (g_pPlayerManager && g_pPlayerManager->getCustomWeaponEquipped()!=0))) Release(owner);
        if(owner) if(auto* entity=blade.getEntity()) {
            bool hidden=g_StaFlags.STA_EVENT||g_StaFlags.STA_QTE||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT;
            if(!hidden) {
                DrawKatana();
                owner->m_CustomWeaponHandle=entity;
                owner->m_CustomWeaponBone=DualBladePolicy::HandBone;
                owner->field_1420=DualBladePolicy::HandBone;
                auto* mount=owner->getConstraintById(DualBladePolicy::Constraint);
                if(!mount || mount->m_MainEntity.getEntity()!=owner->m_pEntity ||
                    mount->m_ObjectEntityHandle.getEntity()!=entity || mount->m_nBone!=DualBladePolicy::HandBone ||
                    mount->m_nRotationBone!=DualBladePolicy::GripBone) {
                    if(!WeaponAttachment::Bind(owner,entity,DualBladePolicy::HandBone,DualBladePolicy::GripBone))
                        Report("wp2050 ERROR: off-hand attachment lost during player update");
                }
            }
            if(entity->m_pBehavior) {
                if(hidden) entity->m_pBehavior->offDisp();
                else {
                    ShowBlade(entity->m_pBehavior);
                }
            }
        }
    }
    void PostTick(Pl0000* player)
    {
        Tick(player);
        if(owner && !(g_StaFlags.STA_EVENT||g_StaFlags.STA_QTE||g_StaFlags.STA_CODEC||g_StaFlags.STA_SOFT_EVENT)) {
            DrawKatana(true);
            if(++diagnosticTicks%120==0 && diagnosticTicks<=1200) {
                auto* e=blade.getEntity();auto* b=e?e->m_pBehavior:nullptr;
                auto* hand=owner->getPartsPtr(DualBladePolicy::HandBone);
                auto* c=owner->getConstraintById(DualBladePolicy::Constraint);
                std::string path=SamConfig::GetIniPath();path.resize(path.find_last_of("\\/")+1);path+="scripts\\DualBladeRuntime.log";
                FILE* f=nullptr;if(b && !fopen_s(&f,path.c_str(),"a") && f) {
                    fprintf(f,"visual: bone=%x rot=%x hand=%p modelFlags=%x objFlags=%x alpha=%.2f/%.2f pos=%.2f,%.2f,%.2f handpos=%.2f,%.2f,%.2f scale=%.2f,%.2f,%.2f meshes=%d first=%x opacity=%.2f\n",
                        c?c->m_nBone:0,c?c->m_nRotationBone:0,hand,b->m_ModelFlags,b->m_ObjectFlags,b->m_CurrentCamAlphaRate,b->m_CamAlphaRate,
                        b->m_LocalMatrix.r[3].x,b->m_LocalMatrix.r[3].y,b->m_LocalMatrix.r[3].z,
                        hand?hand->m_LocalMatrix.r[3].x:0,hand?hand->m_LocalMatrix.r[3].y:0,hand?hand->m_LocalMatrix.r[3].z:0,
                        b->m_Scale.x,b->m_Scale.y,b->m_Scale.z,b->m_MeshNum,b->m_pMeshes[0].m_MeshFlags,b->m_pMeshes[0].m_Color.a);fclose(f);
                }
            }
            if(owner->getConstraintsEntity(4)==owner->m_SwordHandle.getEntity() &&
               owner->getConstraintsEntity(DualBladePolicy::Constraint)==blade.getEntity())
                Report("wp2050 both render models present; hand attachments and drawn blade meshes restored after player update");
        }
    }
};

namespace DualBladeRuntime
{
    using Factory=EntitySystem::EntityCreationData*(__cdecl*)(unsigned);
    using Data=BOOL(__thiscall*)(cObjReadManager*,Hw::cFmerge&,eObjID,int);
    using Ref=void(__thiscall*)(cObjReadManager*,eObjID,int);
    inline Factory originalFactory=nullptr;
    inline Data originalData=nullptr;
    inline Ref originalAddRef=nullptr,originalRemoveRef=nullptr;
    inline EntitySystem::EntityCreationData customFactory{};
    inline std::array<EntitySystem::EntityCreationData,PistolPolicy::Count> pistolFactories{};
    inline EntitySystem::EntityCreationData* __cdecl GetFactory(unsigned id)
    {
        if(PistolPolicy::Object(id)) {
            auto* native=originalFactory(0x32040);
            if(!native)return nullptr;
            auto& factory=pistolFactories[id-PistolPolicy::Get(0).object];
            factory=*native;factory.m_ObjectID=static_cast<eObjID>(id);return &factory;
        }
        if(id==DualBladePolicy::WeaponObject) {
            auto* native=originalFactory(0x32040); // cPl0000Weapon
            if(!native) return nullptr;
            customFactory=*native;customFactory.m_ObjectID=static_cast<eObjID>(id);
            return &customFactory;
        }
        return originalFactory(id);
    }
    inline BOOL __fastcall GetData(cObjReadManager* manager,void*,Hw::cFmerge& output,eObjID id,int set)
    {
        if(PistolPolicy::Object(unsigned(id)))return PistolAssets::Supply(unsigned(id),output)?TRUE:FALSE;
        if(static_cast<unsigned>(id)==DualBladePolicy::WeaponObject)
            return DualBladeWeapon::Get().Supply(output)?TRUE:FALSE;
        return originalData(manager,output,id,set);
    }
    // wp2050 bytes are plugin-owned and retained for the process lifetime.
    // Do not put this unstreamed object through native CPK reference bookkeeping.
    inline void __fastcall AddRef(cObjReadManager* manager,void*,eObjID id,int set)
    {if(static_cast<unsigned>(id)!=DualBladePolicy::WeaponObject && !PistolPolicy::Object(unsigned(id))) originalAddRef(manager,id,set);}
    inline void __fastcall RemoveRef(cObjReadManager* manager,void*,eObjID id,int set)
    {if(static_cast<unsigned>(id)!=DualBladePolicy::WeaponObject && !PistolPolicy::Object(unsigned(id))) originalRemoveRef(manager,id,set);}
    inline void Install()
    {
        static SafeHook::Hook factory((void*)(shared::base+0x68F420),(void*)GetFactory,true,(void**)&originalFactory);
        static SafeHook::Hook data((void*)(shared::base+0x601170),(void*)GetData,true,(void**)&originalData);
        static SafeHook::Hook add((void*)(shared::base+0x5FE710),(void*)AddRef,true,(void**)&originalAddRef);
        static SafeHook::Hook remove((void*)(shared::base+0x5FE7D0),(void*)RemoveRef,true,(void**)&originalRemoveRef);
    }
}
