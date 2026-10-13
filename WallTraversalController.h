#pragma once
#include "WallTraversalPolicy.h"
#include "WallTraversalMotion.h"
#include "SamPlayerIdentity.h"
#include "SamTargets.h"
#include "gui.h"
#include <BehaviorUtility.h>
#include <CharacterControl.h>
#include <cObjReadManager.h>
#include <cGame.h>
#include <cCameraGame.h>
#include <EntitySystem.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <typeinfo>

// Traversal owns a dedicated action, never a collision-off or invulnerability flag.
// Motion copies outlive every animation slot, including scene teardown.
class WallTraversalController {
    using Vec=WallTraversalPolicy::Vec;
    using Contact=WallTraversalPolicy::Contact;
    using Mode=WallTraversalPolicy::Mode;
    using Clip=WallTraversalPolicy::Clip;
    Pl0000* owner=nullptr;
    WallTraversalPolicy::State state;
    Vec anchor{},normal{},tangent{},surfaceOrigin{},planePoint{},launchVel{};
    EntityHandle supportEntity{},launchTarget{};
    bool ascending=false,launchHoming=false,launchStrike=false,strikePending=false,prevLaunchKey=false;
    bool obstacleHop=false,hopAscending=false,slideLatched=false,prevSlideKey=false;
    Vec hopTravel{};
    float savedRoll=0,ownedRoll=0,clearance=.47f,radius=.35f,height=1.8f,entryHeight=0;
    bool nativeSam=false,runHeld=false,clingHeld=false,jump=false,loaded=false,supported=true;
    std::atomic<bool> enabled{true};
    int runKey='V',clingKey='Z',slideKey='X';
    unsigned runLimit=0,clingLimit=0,slideLimit=0;
    float speed=.16f;
    std::vector<unsigned char> runUpMotion,runLeftMotion,runRightMotion,slideUpMotion,slideDownMotion,slideLeftMotion,slideRightMotion,clingMotion,launchMotion;
    Clip activeClip=Clip::None;
    float motionClearance=.47f;
    std::vector<std::vector<unsigned char>> retiredMotions;
    unsigned surfaceQueryLayer=6;
    std::atomic<int> viewMode{0};
    std::atomic<unsigned> viewTicks{0};
    std::atomic<float> viewDistance{0};
    std::atomic<const char*> status{"ready"};
    const char* lastRejected=nullptr;
    DWORD lastRejectTime=0;
    Vec probeNormal{};
    unsigned probeObject=0;
    unsigned probeQueryLayer=6;
    bool probeHit=false;
    uintptr_t probeUserData=0;
    std::vector<Entity*> sceneEntities;
    static Vec V(const Hw::cVec4& p) {return {p.x,p.y,p.z};}
    static Hw::cVec4 H(Vec p,float w=1) {return Hw::cVec4(p.x,p.y,p.z,w);}
    static bool Key(int code) {return code>0&&(GetAsyncKeyState(code)&0x8000)!=0;}
    static bool Focused() {DWORD p=0;GetWindowThreadProcessId(GetForegroundWindow(),&p);return p==GetCurrentProcessId();}
    void Stick(float& sx,float& sy) const {
        sx=0.0f;sy=0.0f;
        const unsigned pad=(owner?owner->m_CurrentInput.m_On:0u)|g_dbPad.m_On;
        if((pad&(Hw::PAD_BTN_LAL|Hw::PAD_BTN_L))!=0)sx-=1.0f;
        if((pad&(Hw::PAD_BTN_LAR|Hw::PAD_BTN_R))!=0)sx+=1.0f;
        if((pad&(Hw::PAD_BTN_LAU|Hw::PAD_BTN_U))!=0)sy+=1.0f;
        if((pad&(Hw::PAD_BTN_LAD|Hw::PAD_BTN_D))!=0)sy-=1.0f;
        if(Key('A')||Key(VK_LEFT))sx=-1.0f;
        else if(Key('D')||Key(VK_RIGHT))sx=1.0f;
        if(Key('S')||Key(VK_DOWN))sy=-1.0f;
        else if(Key('W')||Key(VK_UP))sy=1.0f;
        sx=(std::clamp)(sx,-1.0f,1.0f);
        sy=(std::clamp)(sy,-1.0f,1.0f);
    }
    Vec CameraForward() const {
        Vec cam=V(g_GameCamera.m_NowMatrix.m_Target)-V(g_GameCamera.m_NowMatrix.m_Trans);
        cam={cam.x,0,cam.z};
        if(WallTraversalPolicy::Length(cam)>.05f)return WallTraversalPolicy::Unit(cam);
        if(owner)return {std::sin(owner->m_Rot.y),0,std::cos(owner->m_Rot.y)};
        return {0,0,1};
    }
    Entity* FindLaunchEnemy(float maxRange,bool lockOnly) const {
        if(!owner||!owner->m_pEntity)return nullptr;
        if(g_GameCamera.m_LockonSystem.m_nLockType!=0) {
            Entity* locked=g_GameCamera.m_LockonSystem.m_TargetEntity.getEntity();
            if(!locked&&g_GameCamera.m_LockonSystem.m_pTarget)
                locked=g_GameCamera.m_LockonSystem.m_pTarget->m_pEntity;
            if(locked&&SamTargets::Enemy(locked)&&
                WallTraversalPolicy::Length(V(locked->getTransPos())-anchor)<=maxRange)
                return locked;
        }
        Entity* zTarget=owner->field_1370.m_TargetHandle.getEntity();
        if(zTarget&&SamTargets::Enemy(zTarget)&&
            WallTraversalPolicy::Length(V(zTarget->getTransPos())-anchor)<=maxRange)
            return zTarget;
        if(lockOnly)return nullptr;
        const Vec camF=CameraForward();
        Entity* best=nullptr;float bestScore=maxRange*maxRange*4.0f;
        for(Entity* candidate:sceneEntities) {
            if(!SamTargets::Enemy(candidate))continue;
            Vec p=V(candidate->getTransPos());
            Vec delta=p-anchor;
            float distSq=WallTraversalPolicy::Dot(delta,delta);
            if(distSq>maxRange*maxRange||distSq<.05f)continue;
            if(WallTraversalPolicy::Dot(delta,normal)<-1.5f)continue;
            Vec flatDir=WallTraversalPolicy::Unit({delta.x,0,delta.z});
            float align=WallTraversalPolicy::Dot(flatDir,camF);
            float score=distSq*(1.35f-.55f*align);
            if(score<bestScore){bestScore=score;best=candidate;}
        }
        return best;
    }
    static void Log(const char* event,const char* reason,Vec position={},Vec surface={}) {
        char path[MAX_PATH];if(!GetModuleFileNameA(nullptr,path,MAX_PATH))return;
        char* slash=std::strrchr(path,'\\');if(!slash)return;
        std::strcpy(slash+1,"scripts\\WallTraversal.log");
        FILE* file=nullptr;fopen_s(&file,path,"a");if(!file)return;
        std::fprintf(file,"[%lu] %s: %s pos=(%.3f,%.3f,%.3f) normal=(%.3f,%.3f,%.3f)\n",
            GetTickCount(),event,reason,position.x,position.y,position.z,surface.x,surface.y,surface.z);
        std::fclose(file);
    }
    void Reject(const char* reason) {
        status=reason;
        DWORD now=GetTickCount();
        if(reason!=lastRejected||now-lastRejectTime>=1500) {
            char details[256];std::snprintf(details,sizeof(details),"%s action=%X input=%X ninja-mask=%X hit=%d object=%X user-data=%p query-layer=%u normal=(%.2f,%.2f,%.2f)",
                reason,owner?owner->m_Rno0:0,owner?owner->m_CurrentInput.m_On:0,owner?owner->m_ButtonNinjarun:0,
                int(probeHit),probeObject,reinterpret_cast<void*>(probeUserData),probeQueryLayer,probeNormal.x,probeNormal.y,probeNormal.z);
            Log("entry rejected",details,owner?V(owner->m_pEntity->getTransPos()):Vec{});
            lastRejected=reason;lastRejectTime=now;
        }
    }
    void RefreshEntities() {
        sceneEntities.clear();
        auto& list=g_EntitySystem.getEntityList();auto it=list.begin();
        const int count=(std::clamp)(list.getSize(),0,8192);
        for(int i=0;i<count&&it!=Hw::cFixedList<Entity*>::iterator{};++i,++it)
            if(Entity* entity=*it)sceneEntities.push_back(entity);
    }
    Entity* ResolveSurface(uintptr_t userData) const {
        // Havok user data is opaque. Stage bodies also store non-Entity data here.
        // Only dereference pointers matched against the live native entity list.
        if(!userData)return nullptr;
        for(Entity* entity:sceneEntities)
            if(userData==reinterpret_cast<uintptr_t>(entity)||userData==reinterpret_cast<uintptr_t>(entity->m_pBehavior))return entity;
        return nullptr;
    }
    void Load() {
        if(loaded)return;loaded=true;
        char path[MAX_PATH];GetModuleFileNameA(nullptr,path,MAX_PATH);
        char* slash=std::strrchr(path,'\\');if(!slash)return;
        std::strcpy(slash+1,"SamMoveset.ini");
        auto read=[&](const char* key,int fallback){return int(GetPrivateProfileIntA("WallTraversal",key,fallback,path));};
        enabled=read("Enabled",1)!=0;
        runKey=(std::clamp)(read("RunKey",'V'),0,255);clingKey=(std::clamp)(read("ClingKey",'Z'),0,255);
        slideKey=(std::clamp)(read("SlideKey",'X'),0,255);
        runLimit=unsigned((std::clamp)(read("RunTicks",0),0,36000));
        clingLimit=unsigned((std::clamp)(read("ClingTicks",0),0,36000));
        slideLimit=unsigned((std::clamp)(read("SlideTicks",0),0,36000));
        speed=float((std::clamp)(read("SpeedCentimetresPerTick",16),5,30))*.01f;
        const unsigned char ray[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0};
        const unsigned char position[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0};
        const unsigned char motion[]={0x56,0x8B,0xF1,0x57,0x8B,0xBE,0x8C,0x11,0,0};
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x686290),ray,sizeof(ray)) ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0x4E4580),position,sizeof(position)) ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0x794790),motion,sizeof(motion))) {
            supported=false;enabled=false;status="unsupported executable";
        }
        Log("initialization",supported?(enabled?"enabled build=wall-support-v20":"disabled build=wall-support-v20"):"unsupported executable");
    }
    bool Safe(Pl0000* p) const {
        return p&&p==g_Scene.m_pPlayer&&SamPlayerIdentity::Eligible(p)&&p->m_pCharacterControl&&
            p->m_pCharacterControl->m_pVelocity&&p->isAlive()&&!p->isBladeModeActive()&&
            !gui::IsMenuVisible()&&!g_StaFlags.STA_QTE&&!g_StaFlags.STA_EVENT&&!g_StaFlags.STA_CODEC&&
            !g_StaFlags.STA_SOFT_EVENT&&!g_GameFlags.GAME_SLIDER_NINJARUN_MODE&&
            !g_GameFlags.GAME_MISSILE_NINJYARUN_MODE&&Focused();
    }
    // High 16 bits are the actor exclusion group from Behavior::getCollisionFilter.
    // Keep the native table unchanged; choose a query row that sees this scenery.
    bool CastLayer(Vec start,Vec end,Contact& out,float sphere,unsigned queryLayer) {
        out={};if(!owner||!WallTraversalPolicy::Finite(start)||!WallTraversalPolicy::Finite(end))return false;
        alignas(16) BehaviorUtility::RayCast ray;
        BehaviorUtility::RayInfo info{};
        info.m_TransStart=H(start);info.m_TransEnd=H(end);
        info.m_CollisionFilter=int(queryLayer);info.m_IgnoreObjectFilter=owner->getCollisionFilter();
        info.m_Radius=sphere;info.m_bLinearCast=sphere>0;info.m_bMultiHit=sphere>0?FALSE:TRUE;
        if(!sphere){probeHit=false;probeObject=0;probeNormal={};probeUserData=0;probeQueryLayer=queryLayer;}
        if(!BehaviorUtility::checkRay(&ray,&info))return false;
        if(sphere==0) {
            const auto& hits=ray.m_RayHitCollector.getHits();
            if(!hits.m_data||hits.m_size<=0||hits.m_size>4096)return false;
            float closest=2.0f;
            for(int i=0;i<hits.m_size;++i) {
                const auto& hit=hits.m_data[i];
                const float fraction=hit.m_hitFraction;
                if(!std::isfinite(fraction)||fraction<0||fraction>1||fraction>=closest)continue;
                Vec hitNormal{};std::memcpy(&hitNormal,&hit.m_normal,sizeof(hitNormal));
                Contact candidate=WallTraversalPolicy::RayContact(start,end,hitNormal,fraction,queryLayer);
                if(!candidate.valid)continue;
                // Native collector copies the leaf CdBody at output+0x50.
                // Use the same metadata accessor as single-hit checkRay, never
                // reinterpret metadata flags as an Entity pointer.
                if(!hit.field_50)continue;
                uintptr_t userData=reinterpret_cast<uintptr_t(__cdecl*)(const void*)>(shared::base+0x517CD0)(hit.field_50);
                Entity* entity=ResolveSurface(userData);
                if(entity&&(entity==owner->m_pEntity||!WallTraversalPolicy::Structure(unsigned(entity->m_ObjId))||
                    !entity->isAlive()||entity->m_bDebris||(entity->m_pBehavior&&entity->m_pBehavior->m_pCharacterControl)))continue;
                if(entity) {
                    if(WallTraversalPolicy::ObjectName(entity->m_pName,sizeof(entity->m_pName)))continue;
                    const char* type=entity->m_pBehavior?typeid(*entity->m_pBehavior).name():"";
                    if(WallTraversalPolicy::ObjectName(type,std::strlen(type)))continue;
                    if(WallTraversalPolicy::NameContains(entity->m_pName,sizeof(entity->m_pName),"barricade")||
                       WallTraversalPolicy::NameContains(type,std::strlen(type),"barricade")||
                       WallTraversalPolicy::NameContains(type,std::strlen(type),"barrier")||
                       WallTraversalPolicy::NameContains(entity->m_pName,sizeof(entity->m_pName),"building")||
                       WallTraversalPolicy::NameContains(entity->m_pName,sizeof(entity->m_pName),"facade")||
                       WallTraversalPolicy::NameContains(type,std::strlen(type),"building"))candidate.minimumWallHeight=2.4f;
                }
                // Preserve native rejection of nonphysical/disabled contacts.
                // Broad query coverage must not turn those shapes into support.
                if(!entity&&(userData&0x40000000u))continue;
                if(entity&&(entity==owner->m_pEntity||!WallTraversalPolicy::Structure(unsigned(entity->m_ObjId))||
                    !entity->isAlive()||entity->m_bDebris||(entity->m_pBehavior&&entity->m_pBehavior->m_pCharacterControl)))continue;
                closest=fraction;
                out=candidate;out.surface=reinterpret_cast<uintptr_t>(entity);
                probeHit=true;probeNormal=out.normal;probeUserData=userData;probeObject=entity?unsigned(entity->m_ObjId):0;
            }
            return out.valid;
        }
        out.point=V(ray.m_TransHit);out.normal=V(ray.m_HitDir);
        out.distance=WallTraversalPolicy::Length(out.point-start);
        out.queryLayer=queryLayer;
        Entity* hitEntity=sphere?nullptr:ResolveSurface(reinterpret_cast<uintptr_t>(ray.field_30));
        if(!sphere){probeHit=true;probeNormal=out.normal;probeUserData=reinterpret_cast<uintptr_t>(ray.field_30);probeObject=hitEntity?unsigned(hitEntity->m_ObjId):0;}
        out.valid=WallTraversalPolicy::Finite(out.point)&&WallTraversalPolicy::Finite(out.normal)&&
            WallTraversalPolicy::Length(out.normal)>.8f&&WallTraversalPolicy::Length(out.normal)<1.2f&&
            out.distance<WallTraversalPolicy::Length(end-start)+.1f;
        if(sphere>0) {
            // A character touching a wall must be able to separate from it.
            // Inspect every swept contact; only an outward step from the supporting
            // plane may ignore overlap. Tangential/upward motion is allowed only
            // with body clearance and contacts on that same plane.
            const auto& points=ray.m_CdPointCollector.m_points;
            const auto collection=WallTraversalPolicy::ClassifySweepContacts(points.m_size,points.m_data!=nullptr);
            if(collection==WallTraversalPolicy::SweepContacts::Clear)return false;
            if(collection==WallTraversalPolicy::SweepContacts::Inspect) {
                bool separating=true;
                for(int i=0;i<points.m_size;++i) {
                    Vec contactNormal,point;std::memcpy(&contactNormal,&points.m_data[i].m_contact.m_separatingNormal,sizeof(contactNormal));
                    std::memcpy(&point,&points.m_data[i].m_contact.m_position,sizeof(point));
                    if(!WallTraversalPolicy::SupportingSweepContact(point,contactNormal,planePoint,normal,start,end,sphere)) {separating=false;break;}
                }
                if(separating)return false;
            }
            return true;
        }
        // Scenery with an Entity is still usable; do not require native parkour metadata.
        if(hitEntity) {
            out.surface=reinterpret_cast<uintptr_t>(hitEntity);
            if(hitEntity==owner->m_pEntity||!WallTraversalPolicy::Structure(unsigned(hitEntity->m_ObjId))||
                !hitEntity->isAlive()||hitEntity->m_bDebris||
                (hitEntity->m_pBehavior&&hitEntity->m_pBehavior->m_pCharacterControl))out.valid=false;
        }
        if(out.valid) {
            out.normal=WallTraversalPolicy::Unit(out.normal);
            if(WallTraversalPolicy::Dot(start-out.point,out.normal)<0)out.normal=out.normal*-1;
        }
        return out.valid;
    }
    bool Cast(Vec start,Vec end,Contact& out,float sphere=0) {
        const auto layers=WallTraversalPolicy::QueryLayers(surfaceQueryLayer);
        if(sphere>0) {
            // Check both the supporting surface's row and the original native
            // movement row. A newly detected wall must also block body sweeps.
            if(CastLayer(start,end,out,sphere,layers[0]))return true;
            if(layers[0]!=6&&CastLayer(start,end,out,sphere,6))return true;
            out={};return false;
        }
        for(unsigned layer:layers)
            if(CastLayer(start,end,out,0,layer))return true;
        out={};return false;
    }
    bool LargeSurface(Vec feet,const Contact& contact) {
        const Vec n=WallTraversalPolicy::Unit({contact.normal.x,0,contact.normal.z});
        const Vec side{n.z,0,-n.x};
        const float half=contact.minimumWallHeight*.5f;
        for(float centerY:{feet.y+.1f+half,feet.y+.1f-half}) {
            std::array<Contact,4> edges{};
            const std::array<Vec,4> origins={feet+side*-1.2f,feet+side*1.2f,feet,feet};
            for(size_t i=0;i<edges.size();++i) {
                Vec start=origins[i];start.y=centerY+(i==2?-half:(i==3?half:0.0f));
                Cast(start,start-n*(clearance+.9f),edges[i]);
            }
            if(WallTraversalPolicy::LargeFacade(edges,n,contact.minimumWallHeight))return true;
        }
        return false;
    }
    bool Wall(Vec feet,Vec direction,Contact& out,float reach=1.35f,bool climbing=false) {
        direction=WallTraversalPolicy::Unit({direction.x,0,direction.z});
        if(WallTraversalPolicy::Length(direction)<.5f)return false;
        std::array<Contact,8> samples{};
        const float offsets[]={radius+.15f,height*.55f,height-radius-.10f,.12f};
        for(size_t i=0;i<4;++i) {
            const Vec start=feet+Vec{0,offsets[i],0};
            Cast(start,start+direction*reach,samples[i]);
        }
        // Hands and feet can remain on narrow facade ribs beside a recessed
        // center panel. Keep these probes within the body's lateral reach.
        Vec side{direction.z,0,-direction.x};
        for(size_t i=4;i<samples.size();++i) {
            const float sign=(i%2)?1.0f:-1.0f;
            const float y=i<6?offsets[0]:offsets[1];
            Vec start=feet+side*(sign*.22f)+Vec{0,y,0};
            Cast(start,start+direction*reach,samples[i]);
        }
        if(climbing) {
            if(!WallTraversalPolicy::AttachedSupport(samples,feet,direction*-1,clearance,out))return false;
            out.normal=WallTraversalPolicy::Unit({out.normal.x,0,out.normal.z});
            surfaceQueryLayer=out.queryLayer;
            return WallTraversalPolicy::Length(out.normal)>.5f;
        }
        if(!WallTraversalPolicy::EntryWallSupport(samples,out))return false;
        if(!LargeSurface(feet,out)){out={};return false;}
        surfaceQueryLayer=out.queryLayer;
        return true;
    }
    bool Sweep(Vec from,Vec to) {
        if(WallTraversalPolicy::Length(to-from)<.0001f)return true;
        const float r=radius*.85f;
        for(float y:{radius+.25f,height*.55f,height-radius-.08f}) {
            Contact hit;
            if(Cast(from+Vec{0,y,0},to+Vec{0,y,0},hit,r))return false;
        }
        return true;
    }
    bool TurnCorner(Vec current,Vec next) {
        if(ascending||WallTraversalPolicy::Length({tangent.x,0,tangent.z})<.2f)return false;
        const Vec oldNormal=normal,oldPlane=planePoint;
        const Vec flat=WallTraversalPolicy::Unit({tangent.x,0,tangent.z});
        const float reach=clearance+.75f;
        // Fan toward the adjacent face. Outward-offset probes also see the back
        // of convex corners; all candidate movement remains sphere-swept.
        for(float offset:{0.0f,.24f}) {
            Vec probe=next+oldNormal*offset;
            for(Vec direction:{flat,WallTraversalPolicy::Unit(flat-oldNormal),oldNormal*-1,
                               WallTraversalPolicy::Unit(flat*-1-oldNormal),flat*-1}) {
                Contact hit;
                if(!Wall(probe,direction,hit,reach,false))continue;
                Vec newNormal=WallTraversalPolicy::Unit({hit.normal.x,0,hit.normal.z});
                const float alignment=WallTraversalPolicy::Dot(newNormal,oldNormal);
                if(alignment<-.05f||alignment>.96f)continue;
                Vec target=next+newNormal*(clearance-WallTraversalPolicy::Dot(next-hit.point,newNormal));
                if(WallTraversalPolicy::Length(target-current)>.65f)continue;
                // Both clearance planes must be respected at an inside corner.
                normal=newNormal;planePoint=hit.point;
                if(!Sweep(current,target)) {normal=oldNormal;planePoint=oldPlane;continue;}
                Contact support;
                if(!Wall(target,newNormal*-1,support,reach,true)) {normal=oldNormal;planePoint=oldPlane;continue;}
                Contact middleSupport;
                const Vec middle=(current+target)*.5f;
                if(!Wall(middle,oldNormal*-1,middleSupport,clearance+.14f,true)&&
                   !Wall(middle,newNormal*-1,middleSupport,clearance+.14f,true)) {normal=oldNormal;planePoint=oldPlane;continue;}
                tangent=WallTraversalPolicy::TransportTravel(oldNormal,newNormal,tangent);
                supportEntity=reinterpret_cast<Entity*>(support.surface);
                if(Entity* structure=supportEntity.getEntity())surfaceOrigin=V(structure->getTransPos());
                auto position=H(target);owner->setTransPos(position);
                owner->m_pCharacterControl->setPosition(position,TRUE);owner->m_pEntity->setTransPos(position);
                anchor=target;ZeroVelocity();Orient();
                if(!Play(false)){End("motion unavailable");return true;}
                if(state.Advance(runLimit,clingLimit,slideLimit)) {
                    End("traversal time limit");return true;
                }
                viewTicks=state.ticks;
                Log("corner","continued onto adjacent wall",anchor,normal);
                return true;
            }
        }
        normal=oldNormal;planePoint=oldPlane;return false;
    }
    bool Roof(Vec feet,Contact& floor,Vec& landing) {
        Vec inside=feet-normal*(clearance+radius+.16f);
        Contact wallBlock;
        if(Cast(feet+Vec{0,.65f,0},inside+Vec{0,.65f,0},wallBlock))return false;
        if(!Cast(inside+Vec{0,.45f,0},inside-Vec{0,.35f,0},floor)||!WallTraversalPolicy::Top(floor,feet))return false;
        landing={inside.x,floor.point.y+.06f,inside.z};return true;
    }
    bool FinishTop(Vec feet) {
        if(feet.y-entryHeight<.75f&&state.ticks<5u)return false;
        // Two-contact support ends slightly before the feet reach a roof.
        // A small mantle is permitted only after finding a real walkable top
        // and clearing both the vertical and inward legs with body sweeps.
        for(float rise:{0.0f,.16f,.32f,.48f}) {
            Contact floor;Vec landing;Vec raised=feet+Vec{0,rise,0};
            if(!Roof(raised,floor,landing)||!Sweep(V(owner->m_pEntity->getTransPos()),raised)||!Sweep(raised,landing))continue;
            auto position=H(landing);owner->setTransPos(position);owner->m_pCharacterControl->setPosition(position,TRUE);
            owner->m_pEntity->setTransPos(position);anchor=landing;
            End("reached structure top");return true;
        }
        return false;
    }
    bool Motions() {
        if(owner&&owner->m_pCharacterControl) {
            const float desired=(std::max)((std::clamp)(owner->m_pCharacterControl->m_Radius,.20f,.50f)+.12f,.47f);
            if(std::fabs(desired-motionClearance)>.001f) {
                // Retain allocations still referenced by native animation slots.
                for(auto* motion:{&runUpMotion,&runLeftMotion,&runRightMotion,&slideUpMotion,&slideDownMotion,
                                  &slideLeftMotion,&slideRightMotion,&clingMotion,&launchMotion})
                    if(!motion->empty())retiredMotions.push_back(std::move(*motion));
                motionClearance=desired;activeClip=Clip::None;
            }
            clearance=desired;
        }
        if(!runUpMotion.empty()&&!runLeftMotion.empty()&&!runRightMotion.empty()&&
           !slideUpMotion.empty()&&!slideDownMotion.empty()&&!slideLeftMotion.empty()&&!slideRightMotion.empty()&&
           !clingMotion.empty()&&!launchMotion.empty())return true;
        Hw::cFmerge archive;
        if(!g_ObjReadManager.getDataAtSet(archive,static_cast<eObjID>(0x10010),0))return false;
        auto load=[&](std::vector<unsigned char>& out,const char* name,WallTraversalMotion::Variant v,const char* fallback=nullptr) {
            if(!out.empty())return;
            const void* raw=archive.getFileNameData(name);
            size_t sz=raw?archive.getFileNameSize(name,0):0;
            if((!raw||!sz)&&fallback) {raw=archive.getFileNameData(fallback);sz=raw?archive.getFileNameSize(fallback,0):0;}
            if(raw&&sz)out=WallTraversalMotion::CopyMotion(raw,sz,v,motionClearance);
        };
        // Keep these owned copies for process lifetime; native animation slots may still reference them.
        load(runUpMotion,"pl0010_6011.mot",WallTraversalMotion::Variant::RunUp);
        load(runLeftMotion,"pl0010_6012.mot",WallTraversalMotion::Variant::RunSideLeft);
        load(runRightMotion,"pl0010_6013.mot",WallTraversalMotion::Variant::RunSideRight);
        load(slideUpMotion,"pl0010_6001.mot",WallTraversalMotion::Variant::RunUp,"pl0010_6011.mot");
        load(slideDownMotion,"pl0010_6022.mot",WallTraversalMotion::Variant::SlideDown,"pl0010_60c1.mot");
        load(slideLeftMotion,"pl0010_d152.mot",WallTraversalMotion::Variant::SlideSideLeft,"pl0010_6012.mot");
        load(slideRightMotion,"pl0010_d153.mot",WallTraversalMotion::Variant::SlideSideRight,"pl0010_6013.mot");
        load(clingMotion,"pl0010_0000.mot",WallTraversalMotion::Variant::Cling);
        load(launchMotion,"pl0010_6021.mot",WallTraversalMotion::Variant::Launch,"pl0010_0201.mot");
        if(launchMotion.empty())launchMotion=runUpMotion;
        return !runUpMotion.empty()&&!runLeftMotion.empty()&&!runRightMotion.empty()&&
               !slideUpMotion.empty()&&!slideDownMotion.empty()&&!slideLeftMotion.empty()&&!slideRightMotion.empty()&&
               !clingMotion.empty()&&!launchMotion.empty();
    }
    std::vector<unsigned char>& MotionFor(Clip clip) {
        switch(clip) {
        case Clip::RunUp:return runUpMotion;
        case Clip::RunSideLeft:return runLeftMotion;
        case Clip::RunSideRight:return runRightMotion;
        case Clip::SlideDown:return slideDownMotion;
        case Clip::SlideUp:return slideUpMotion;
        case Clip::SlideUpLeft:return runLeftMotion;
        case Clip::SlideUpRight:return runRightMotion;
        case Clip::SlideSideLeft:return slideLeftMotion;
        case Clip::SlideSideRight:return slideRightMotion;
        case Clip::Launch:return launchMotion;
        default:return clingMotion;
        }
    }
    void Orient() {
        const Clip clip=WallTraversalPolicy::SelectClip(state.mode,ascending,normal,tangent);
        Vec facing{};
        if(state.mode==Mode::Launch) {
            facing=WallTraversalPolicy::Unit({launchVel.x,0,launchVel.z});
            if(WallTraversalPolicy::Length(facing)<.05f)facing=normal;
        } else if(state.mode==Mode::Cling||ascending||clip==Clip::SlideDown) {
            facing=normal*-1;
        } else {
            facing=WallTraversalPolicy::Unit({tangent.x,0,tangent.z});
        }
        owner->m_Rot.y=std::atan2(facing.x,facing.z);
        ownedRoll=0.0f;
        owner->m_Rot.z=ownedRoll;
    }
    bool Play(bool force=false) {
        const Clip clip=WallTraversalPolicy::SelectClip(state.mode,ascending,normal,tangent);
        if(clip==Clip::None)return false;
        if(!force&&clip==activeClip)return true;
        auto& motion=MotionFor(clip);
        if(motion.empty())return false;
        // 0x30000 = YTranslateEaseOff | ZTranslateEaseOff with loop enabled; 0x8000000 = one-shot pose for Cling/Launch.
        const unsigned flags=(clip==Clip::Cling||clip==Clip::Launch)?0x8000000u:0x30000u;
        const float rate=clip==Clip::Cling?0.0f:(clip==Clip::Launch?1.25f:1.0f);
        const float blend=activeClip==Clip::None?.04f:.08f;
        if(owner->setDirectAnimation(motion.data(),nullptr,0,blend,1.0f,flags,0,rate)==-1)return false;
        activeClip=clip;
        Orient();
        if(!g_StaFlags.STA_PAUSE&&!Trigger::StpFlags.STP_OBJ)
            reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(owner,1.0f,1.0f);
        return true;
    }
    const char* StatusText() const {
        if(state.mode==Mode::Launch)return obstacleHop?"jumping over wall obstacle":(launchHoming?"launching to enemy":"launching from wall");
        if(state.mode==Mode::Cling)return "hanging on wall";
        if(state.mode==Mode::Slide) {
            if(tangent.y>.15f)return "sliding up structure";
            return WallTraversalPolicy::SelectClip(state.mode,ascending,normal,tangent)==Clip::SlideDown
                ?"sliding down structure":"sliding alongside structure";
        }
        return ascending?"running up structure":"running alongside structure";
    }
    void ZeroVelocity() {
        owner->m_TransSpeed=H({},0);owner->field_1020=H({},0);
        *owner->m_pCharacterControl->m_pVelocity=H({},0);
    }
    void ApplyVelocity(Vec v) {
        owner->m_TransSpeed=H(v,0);owner->field_1020=H(v,0);
        *owner->m_pCharacterControl->m_pVelocity=H(v,0);
    }
    void ClaimAction(Pl0000* player) {
        if(!player || player->m_Rno0==WallTraversalPolicy::Action)return;
        // Native graph cleanup expects the graph as this and its state context
        // on the stack, matching SamNativeRuntime::ClearStates.
        auto* states=reinterpret_cast<void**>(reinterpret_cast<char*>(player)+0x7CC);
        if(states[0] && states[1])
            reinterpret_cast<void(__thiscall*)(void*,void*)>(shared::base+0x982DE0)(states[0],states[1]);
        player->setRno(WallTraversalPolicy::Action,0,0,0);
    }
    bool BeginWallLaunch(bool forceEnemySearch,bool strikeOnArrival) {
        obstacleHop=false;
        RefreshEntities();
        float sx=0,sy=0;Stick(sx,sy);
        Entity* enemy=FindLaunchEnemy(forceEnemySearch?28.0f:22.0f,!forceEnemySearch);
        if(enemy) {
            launchTarget=enemy;
            launchHoming=true;
            launchStrike=true;
            Vec targetPos=V(enemy->getTransPos())+Vec{0,.80f,0};
            launchVel=WallTraversalPolicy::HomingLaunchStep(anchor,targetPos,normal,0,.65f);
        } else {
            launchTarget=nullptr;
            launchHoming=false;
            launchStrike=strikeOnArrival;
            launchVel=WallTraversalPolicy::WallLaunchVelocity(normal,CameraForward(),tangent,sx,sy);
        }
        ascending=false;
        state.Switch(Mode::Launch);
        if(!Play(true)){End("motion unavailable");return false;}
        status=StatusText();viewMode=int(state.mode);
        Log("launch",StatusText(),anchor,normal);
        return true;
    }
    bool BeginObstacleHop() {
        if(state.mode!=Mode::Run)return false;
        if(Sweep(anchor,anchor+tangent*(speed*1.5f)))return false;
        Vec predicted=anchor;
        for(unsigned tick=0;tick<12;++tick) {
            Vec next=predicted+WallTraversalPolicy::ObstacleHopStep(normal,tangent,tick);
            if(!Sweep(predicted,next))return false;
            predicted=next;
        }
        Contact landingSupport;
        if(!Wall(predicted,normal*-1,landingSupport,clearance+.14f,true))return false;
        hopTravel=tangent;hopAscending=ascending;obstacleHop=true;
        launchHoming=false;launchStrike=false;launchTarget=nullptr;
        launchVel=WallTraversalPolicy::ObstacleHopStep(normal,hopTravel,0);
        ascending=false;state.Switch(Mode::Launch);
        if(!Play(true)){End("motion unavailable");return false;}
        viewMode=int(state.mode);status=StatusText();
        Log("hop","cleared obstacle jump with supported destination",anchor,normal);
        return true;
    }
    void End(const char* reason,bool kick=false,bool restoreAction=true) {
        if(!state.Active()){status=reason;activeClip=Clip::None;return;}
        const Mode endedMode=state.mode;
        const bool owns=owner&&owner==g_Scene.m_pPlayer&&owner->m_Rno0==WallTraversalPolicy::Action;
        if(owns&&restoreAction) {
            if(owner->m_Rot.z==ownedRoll)owner->m_Rot.z=savedRoll;
            Contact ground;
            const bool onFloor=Cast(anchor+Vec{0,.08f,0},anchor-Vec{0,.12f,0},ground)&&
                WallTraversalPolicy::Top(ground,anchor);
            bool air=endedMode==Mode::Launch||!onFloor;
            owner->setRno(nativeSam?(air?0x100005u:0x100000u):(air?0xBu:0u),0,0,0);
            if(endedMode==Mode::Launch) {
                ApplyVelocity(launchVel);
            } else if(kick) {
                Vec impulse=normal*.28f+Vec{0,.34f,0};
                ApplyVelocity(impulse);
            } else if(air) {
                ApplyVelocity(normal*.04f+Vec{0,-.06f,0});
            }
        } else if(owner&&owner==g_Scene.m_pPlayer&&owner->m_Rot.z==ownedRoll)owner->m_Rot.z=savedRoll;
        // End one traversal without leaving a release latch or query cache that
        // can prevent a fresh attempt against the next building.
        state.End(false,6u);supportEntity=nullptr;launchTarget=nullptr;launchHoming=false;surfaceQueryLayer=6;obstacleHop=false;slideLatched=false;
        activeClip=Clip::None;viewMode=0;viewTicks=0;status=reason;
        Log("exit",reason,anchor,normal);
    }
public:
    static WallTraversalController& Get() {static WallTraversalController instance;return instance;}
    bool Active() const {return state.Active();}
    const char* Status() const {return status.load();}
    void View(int& mode,unsigned& ticks,float& distance) const {mode=viewMode.load();ticks=viewTicks.load();distance=viewDistance.load();}
    bool Enabled() const {return enabled.load()&&supported;}
    void Enable(bool value) {enabled=value&&supported;}
    void Forget(Pl0000* player) {
        if(owner!=player)return;
        // Called before player shutdown; relinquish pose, then never retain its allocation.
        End("player released");owner=nullptr;state={};activeClip=Clip::None;strikePending=false;
    }
    bool HandleAction(Pl0000* player) {
        if(strikePending&&player==owner) {
            player->m_CurrentInput.m_Trig|=player->m_ButtonLightAttack;
            player->m_CurrentInput.m_On|=player->m_ButtonLightAttack;
            strikePending=false;
        }
        if(player!=owner||!state.Active())return false;
        if(player->m_Rno0!=WallTraversalPolicy::Action) {
            if(WallTraversalPolicy::EntryAction(player->m_Rno0))ClaimAction(player);
            else return false;
        }
        if(!Safe(player)){End("gameplay interrupted");return false;}
        if(!g_StaFlags.STA_PAUSE&&!Trigger::StpFlags.STP_OBJ)
            reinterpret_cast<void(__thiscall*)(Pl0000*,float,float)>(shared::base+0x794790)(player,1.0f,1.0f);
        return true;
    }
    void Tick(Pl0000* player,bool sam,bool busy) {
        Load();
        if(owner!=player) {
            // The former player may already be destroyed: do not dereference it.
            owner=nullptr;state={};supportEntity=nullptr;launchTarget=nullptr;activeClip=Clip::None;surfaceQueryLayer=6;
            viewMode=0;viewTicks=0;strikePending=false;obstacleHop=false;slideLatched=false;prevSlideKey=false;owner=player;
        }
        if(!player)return;
        const unsigned on=player->m_CurrentInput.m_On|player->m_CurrentInput.m_Trig|g_dbPad.m_On|g_dbPad.m_Trig;
        const bool ninja=(on&player->m_ButtonNinjarun)!=0;
        const bool explicitRun=Key(runKey);
        const bool yKey=Key('Y');
        const bool secondaryTrig=(player->m_CurrentInput.m_Trig&player->m_ButtonHeavyAttack)!=0||
            (g_dbPad.m_Trig&Hw::PAD_BTN_Y)!=0||(yKey&&!prevSlideKey);
        prevSlideKey=yKey;
        const bool explicitSlide=Key(slideKey)||yKey;
        runHeld=explicitRun||ninja||explicitSlide||(slideLatched&&state.Active());
        clingHeld=Key(clingKey)||(ninja&&(g_dbPad.m_On&Hw::PAD_BTN_L3)!=0);
        jump=(player->m_CurrentInput.m_Trig&player->m_ButtonJump)!=0;
        const bool fKey=Key('F');
        const bool fTrig=fKey&&!prevLaunchKey;
        prevLaunchKey=fKey;
        const bool attackTrig=((player->m_CurrentInput.m_Trig&player->m_ButtonLightAttack)!=0)||fTrig;
        if(!enabled||!Safe(player)||busy) {End(enabled?"gameplay interrupted":"disabled");return;}
        if(g_StaFlags.STA_PAUSE||Trigger::StpFlags.STP_OBJ)return;
        Motions();
        if(state.Active()) {
            if(player->m_Rno0!=WallTraversalPolicy::Action) {
                if(sam==nativeSam&&WallTraversalPolicy::EntryAction(player->m_Rno0))ClaimAction(player);
                else {End("native action took over",false,false);return;}
            }
            if(sam!=nativeSam){End("native action took over",false,false);return;}
            if(secondaryTrig)slideLatched=true;
            if(state.mode==Mode::Launch) {
                if(!launchHoming&&attackTrig) {
                    RefreshEntities();
                    if(Entity* enemy=FindLaunchEnemy(28.0f,false)) {
                        launchTarget=enemy;launchHoming=true;launchStrike=true;obstacleHop=false;
                        status=StatusText();
                    } else {
                        strikePending=true;
                        End("aerial attack from wall launch",false,true);
                        return;
                    }
                }
                ZeroVelocity();return;
            }
            if(attackTrig) {
                BeginWallLaunch(true,true);
                ZeroVelocity();return;
            }
            if(jump) {
                if(BeginObstacleHop()){ZeroVelocity();return;}
                BeginWallLaunch(false,false);
                ZeroVelocity();return;
            }
            if(!runHeld&&!clingHeld&&!secondaryTrig){End("released wall");return;}
            float sx=0,sy=0;Stick(sx,sy);
            if((explicitSlide||secondaryTrig)&&sy<.25f)sy=-1.0f;
            if(secondaryTrig)state.Switch(Mode::Slide);
            const auto steered=WallTraversalPolicy::SteerOnWall(
                state.mode,tangent,ascending,normal,CameraForward(),sx,sy,clingHeld,state.ticks);
            const bool modeChanged=steered.mode!=state.mode;
            tangent=steered.tangent;
            ascending=steered.ascending;
            if(modeChanged)state.Switch(steered.mode);
            if(state.mode!=Mode::Slide){slideLatched=false;}
            if(!Play(modeChanged)){End("motion unavailable");return;}
            status=StatusText();viewMode=int(state.mode);
            ZeroVelocity();return;
        }
        if(jump||explicitRun||(!state.cooldown&&player->isInAir()==FALSE))state.requireRelease=false;
        state.TickIdle(runHeld,clingHeld);
        if(!runHeld&&!clingHeld)return;
        if(!WallTraversalPolicy::EntryAction(player->m_Rno0)){Reject("native action not eligible");return;}
        if(!explicitRun&&!explicitSlide&&!clingHeld&&(player->m_Rno0==0u||player->m_Rno0==0xDu||player->m_Rno0==0xEu||player->m_Rno0==0x100000u||player->m_Rno0==0x100004u))return;
        if(state.cooldown){Reject("reattachment cooldown");return;}
        if(state.requireRelease){Reject("release Ninja Run before reattaching");return;}
        if(!Motions()){Reject("waiting for Raiden motion bank");return;}
        RefreshEntities();
        radius=(std::clamp)(player->m_pCharacterControl->m_Radius,.20f,.50f);
        height=(std::clamp)(player->m_pCharacterControl->m_Height,1.3f,2.2f);
        clearance=(std::max)(radius+.12f,.47f);
        Vec feet=V(player->m_pEntity->getTransPos()),forward{std::sin(player->m_Rot.y),0,std::cos(player->m_Rot.y)};
        Contact best;float nearest=2.5f;
        Vec right{forward.z,0,-forward.x};
        const std::array<Vec,5> probes=(explicitRun||explicitSlide||clingHeld)
            ?std::array<Vec,5>{forward,WallTraversalPolicy::Unit(forward+right*.55f),WallTraversalPolicy::Unit(forward-right*.55f),right,right*-1}
            :std::array<Vec,5>{forward,WallTraversalPolicy::Unit(forward+right*.45f),WallTraversalPolicy::Unit(forward-right*.45f),right,right*-1};
        for(Vec d:probes) {
            Contact hit;if(Wall(feet,d,hit,1.45f,false)&&hit.distance<nearest){best=hit;nearest=hit.distance;}
        }
        if(!best.valid){Reject("no suitable structure wall");return;}
        // The last successful fan probe need not be the selected closest wall.
        surfaceQueryLayer=best.queryLayer;
        const bool closeParallel=WallTraversalPolicy::Dot(feet-best.point,best.normal)<=clearance+.14f;
        if(!WallTraversalPolicy::HasEntryIntent(player->m_Rno0,best.normal,forward,explicitRun||explicitSlide,clingHeld,closeParallel))return;
        // Refine along the wall normal so angled probes lock onto the true surface plane directly facing the player.
        Vec n=WallTraversalPolicy::Unit({best.normal.x,0,best.normal.z});
        Contact perp;
        if(Wall(feet,n*-1,perp,1.45f,true)) {
            best=perp;
            n=WallTraversalPolicy::Unit({best.normal.x,0,best.normal.z});
        }
        float distance=WallTraversalPolicy::Dot(feet-best.point,n);
        if(!WallTraversalPolicy::EntryDistance(distance,radius,clearance)){Reject("wall outside grab distance");return;}
        Vec entry=feet+n*(clearance-distance);
        if(!perp.valid) {
            Vec wallAnchor{best.point.x,feet.y,best.point.z};
            Vec desired=wallAnchor+n*clearance;
            Vec step=desired-feet;
            step.y=0;
            if(WallTraversalPolicy::Length(step)<=.65f)entry=desired;
        }
        // Supply the entry plane so outward movement can escape existing wall contact.
        normal=n;planePoint=best.point;
        if(!Sweep(feet,entry)){Reject("entry obstructed");return;}
        Contact entrySupport;
        if(!Wall(entry,n*-1,entrySupport,clearance+.14f,true)){Reject("no close body support");return;}
        if(!state.Begin(clingHeld?Mode::Cling:(explicitSlide?Mode::Slide:Mode::Run)))return;
        slideLatched=explicitSlide;
        anchor=entry;entryHeight=entry.y;normal=n;tangent=WallTraversalPolicy::Travel(normal,forward);ascending=tangent.y>.5f;
        if(explicitSlide&&!clingHeld) {
            float sx=0,sy=0;Stick(sx,sy);
            if(sy>=.25f) {
                const auto steer=WallTraversalPolicy::SteerOnWall(Mode::Slide,tangent,ascending,normal,CameraForward(),sx,sy,false,0);
                tangent=steer.tangent;ascending=steer.ascending;
            } else {tangent=WallTraversalPolicy::SlideTravel(ascending,tangent);ascending=false;}
        }
        supportEntity=reinterpret_cast<Entity*>(best.surface);
        if(Entity* structure=supportEntity.getEntity())surfaceOrigin=V(structure->getTransPos());
        savedRoll=ownedRoll=player->m_Rot.z;nativeSam=sam;activeClip=Clip::None;
        auto entryPos=H(entry);player->setTransPos(entryPos);player->m_pCharacterControl->setPosition(entryPos,TRUE);player->m_pEntity->setTransPos(entryPos);
        ClaimAction(player);
        if(!Play(true)){End("motion unavailable");return;}
        ZeroVelocity();status=StatusText();viewMode=int(state.mode);
        Log("entry",state.mode==Mode::Run?(ascending?"upward run":"sideways run"):"cling",anchor,normal);
        char queryDetails[80];std::snprintf(queryDetails,sizeof(queryDetails),"structure acquired using Havok query layer %u",surfaceQueryLayer);
        Log("collision",queryDetails,anchor,normal);
    }
    void PostTick(Pl0000* player) {
        if(player!=owner||!state.Active())return;
        if(player->m_Rno0!=WallTraversalPolicy::Action) {
            if(WallTraversalPolicy::EntryAction(player->m_Rno0))ClaimAction(player);
            else {End("native action took over",false,false);return;}
        }
        if(!Safe(player)){End("native action took over",false,false);return;}
        if(g_StaFlags.STA_PAUSE||Trigger::StpFlags.STP_OBJ)return;
        RefreshEntities();
        const Vec current=V(player->m_pEntity->getTransPos());
        if(WallTraversalPolicy::Length(current-anchor)>1.15f){End("unexpected native displacement");return;}
        if(state.mode==Mode::Launch) {
            if(obstacleHop) {
                launchVel=WallTraversalPolicy::ObstacleHopStep(normal,hopTravel,state.ticks);
                Vec next=anchor+launchVel;
                if(!Sweep(current,next)){End("obstacle jump blocked");return;}
                auto position=H(next);player->setTransPos(position);
                player->m_pCharacterControl->setPosition(position,TRUE);player->m_pEntity->setTransPos(position);
                anchor=next;ZeroVelocity();Orient();
                if(state.Advance(runLimit,clingLimit,slideLimit,12)) {
                    Contact support;
                    if(!Wall(anchor,normal*-1,support,clearance+.14f,true)){End("obstacle jump lost wall");return;}
                    planePoint=support.point;supportEntity=reinterpret_cast<Entity*>(support.surface);
                    if(Entity* structure=supportEntity.getEntity())surfaceOrigin=V(structure->getTransPos());
                    tangent=hopTravel;ascending=hopAscending;obstacleHop=false;
                    if(slideLatched){tangent=WallTraversalPolicy::SlideTravel(ascending,tangent);ascending=false;}
                    state.Switch(slideLatched?Mode::Slide:Mode::Run);
                    if(!Play(true)){End("motion unavailable");return;}
                    Log("hop","resumed supported wall traversal",anchor,normal);
                }
                viewMode=int(state.mode);viewTicks=state.ticks;status=StatusText();return;
            }
            if(launchHoming) {
                Entity* enemy=launchTarget.getEntity();
                if(!enemy||!SamTargets::Enemy(enemy)) {
                    launchHoming=false;
                } else {
                    Vec targetPos=V(enemy->getTransPos())+Vec{0,.80f,0};
                    Vec toEnemy=targetPos-anchor;
                    float dist=WallTraversalPolicy::Length(toEnemy);
                    if(dist<=1.90f) {
                        launchVel=WallTraversalPolicy::Unit(toEnemy)*.24f;
                        strikePending=launchStrike;
                        End("reached enemy from wall launch",false,true);
                        return;
                    }
                    launchVel=WallTraversalPolicy::HomingLaunchStep(anchor,targetPos,normal,state.ticks,.65f);
                }
            }
            Vec next=anchor+launchVel;
            if(!launchHoming) {
                launchVel.y-=.018f;
                if(state.ticks>=5u&&(runHeld||clingHeld)) {
                    Vec flatDir=WallTraversalPolicy::Unit({launchVel.x,0,launchVel.z});
                    Contact hit;
                    if(Wall(anchor,flatDir,hit,clearance+.55f,false)&&(state.ticks>=10u||WallTraversalPolicy::Dot(hit.normal,normal)<.85f)) {
                        normal=WallTraversalPolicy::Unit({hit.normal.x,0,hit.normal.z});planePoint=hit.point;
                        Vec attach=anchor+normal*WallTraversalPolicy::Correction(hit,anchor,clearance);
                        Contact attachSupport;
                        if(Wall(attach,normal*-1,attachSupport,clearance+.14f,true)&&Sweep(current,attach)) {
                            anchor=attach;entryHeight=anchor.y;tangent=WallTraversalPolicy::Travel(normal,flatDir);ascending=tangent.y>.5f;
                            supportEntity=reinterpret_cast<Entity*>(hit.surface);
                            if(Entity* structure=supportEntity.getEntity())surfaceOrigin=V(structure->getTransPos());
                            if(slideLatched){tangent=WallTraversalPolicy::SlideTravel(ascending,tangent);ascending=false;}
                            state.Switch(clingHeld?Mode::Cling:(slideLatched?Mode::Slide:Mode::Run));
                            auto pos=H(anchor);player->setTransPos(pos);player->m_pCharacterControl->setPosition(pos,TRUE);player->m_pEntity->setTransPos(pos);
                            ZeroVelocity();if(!Play(true)){End("motion unavailable");return;}
                            status=StatusText();viewMode=int(state.mode);viewTicks=state.ticks;
                            Log("entry","wall-to-wall chain",anchor,normal);
                            return;
                        }
                    }
                }
                if(launchVel.y<=0.0f) {
                    Contact floor;
                    if(Cast(next+Vec{0,.35f,0},next-Vec{0,.18f,0},floor)&&WallTraversalPolicy::Top(floor,next)) {
                        Vec ground{next.x,floor.point.y+.02f,next.z};
                        auto pos=H(ground);player->setTransPos(pos);player->m_pCharacterControl->setPosition(pos,TRUE);player->m_pEntity->setTransPos(pos);
                        anchor=ground;launchVel={};End("landed from wall launch");return;
                    }
                }
            }
            if(!Sweep(current,next)) {
                if(launchHoming)strikePending=launchStrike;
                End("wall launch obstructed",false,true);
                return;
            }
            auto position=H(next);
            player->setTransPos(position);player->m_pCharacterControl->setPosition(position,TRUE);player->m_pEntity->setTransPos(position);
            anchor=next;ZeroVelocity();Orient();
            if(state.Advance(runLimit,clingLimit,slideLimit,launchHoming?42u:22u)) {
                if(launchHoming)strikePending=launchStrike;
                End("wall launch completed",false,true);
                return;
            }
            viewMode=int(state.mode);viewTicks=state.ticks;status=StatusText();
            return;
        }
        if(supportEntity.m_Handle) {
            Entity* structure=supportEntity.getEntity();
            if(!structure||!structure->isAlive()||WallTraversalPolicy::Length(V(structure->getTransPos())-surfaceOrigin)>.03f) {
                End("structure moved or disappeared");return;
            }
        }
        Contact wall;
        const bool climbing=(ascending||tangent.y>.15f)&&(state.mode==Mode::Run||state.mode==Mode::Slide);
        const float reach=clearance+.95f;
        if(!Wall(anchor,normal*-1,wall,reach,true)||WallTraversalPolicy::Dot(wall.normal,normal)<.70f) {
            if(climbing&&FinishTop(anchor))return;
            if(TurnCorner(current,anchor+tangent*(speed*.75f)))return;
            End("wall ended");return;
        }
        // Smoothly adapt to faceted/curved walls while preserving travel direction.
        Vec adaptedNormal=WallTraversalPolicy::Unit({wall.normal.x,0,wall.normal.z});
        if(WallTraversalPolicy::Length(adaptedNormal)>.5f&&WallTraversalPolicy::Dot(adaptedNormal,normal)>.82f) {
            normal=adaptedNormal;
            if(!ascending&&state.mode==Mode::Run) {
                Vec flatT=WallTraversalPolicy::Tangent(normal,tangent);
                tangent=WallTraversalPolicy::Unit({flatT.x,tangent.y,flatT.z});
            }
        }
        viewDistance=wall.distance;
        planePoint=wall.point;
        Vec next=anchor;
        if(state.mode==Mode::Run)next=next+tangent*speed;
        else if(state.mode==Mode::Slide)next=next+tangent*(speed*.68f);
        next=next+normal*WallTraversalPolicy::Correction(wall,next,clearance);
        if(state.mode==Mode::Slide&&tangent.y<0.0f) {
            Contact floor;
            if(Cast(next+Vec{0,.25f,0},next-Vec{0,.10f,0},floor)&&WallTraversalPolicy::Top(floor,next)) {
                Vec ground{next.x,floor.point.y+.02f,next.z};
                auto pos=H(ground);player->setTransPos(pos);player->m_pCharacterControl->setPosition(pos,TRUE);player->m_pEntity->setTransPos(pos);
                anchor=ground;End("landed from wall slide");return;
            }
        }
        Contact ahead;
        if(!Wall(next,normal*-1,ahead,reach,true)||WallTraversalPolicy::Dot(ahead.normal,normal)<.70f) {
            if(climbing&&FinishTop(next))return;
            if(climbing&&FinishTop(anchor))return;
            if(TurnCorner(current,next))return;
            if(BeginObstacleHop()){ZeroVelocity();return;}
            End("ledge or corner");return;
        }
        if(ahead.distance<wall.distance) {
            planePoint=ahead.point;
            next=next+normal*WallTraversalPolicy::Correction(ahead,next,clearance);
        }
        if(!Sweep(current,next)) {
            if(BeginObstacleHop()){ZeroVelocity();return;}
            if(TurnCorner(current,next))return;
            bool stepped=false;
            for(float push:{.12f,.24f,.38f,.52f}) {
                Vec candidate=next+normal*push;
                Contact candidateSupport;
                if(Wall(candidate,normal*-1,candidateSupport,clearance+.14f,true)&&Sweep(current,candidate)) {next=candidate;stepped=true;break;}
            }
            if(!stepped){End("movement obstructed");return;}
        }
        Contact planted;
        if(!Wall(next,normal*-1,planted,clearance+.14f,true)){End("lost foot contact");return;}
        Vec aligned=next+normal*(clearance-WallTraversalPolicy::Dot(next-planted.point,normal));
        if(!Sweep(current,aligned)){End("foot alignment obstructed");return;}
        next=aligned;planePoint=planted.point;
        auto position=H(next);
        player->setTransPos(position);player->m_pCharacterControl->setPosition(position,TRUE);player->m_pEntity->setTransPos(position);
        anchor=next;ZeroVelocity();
        if(state.ticks%30==0)Log("progress",StatusText(),anchor,normal);
        Orient();
        if(state.Advance(runLimit,clingLimit,slideLimit)) {
            if(state.mode==Mode::Run) {
                tangent=WallTraversalPolicy::SlideTravel(ascending,tangent);ascending=false;
                state.Switch(Mode::Slide);
                if(!Play(true)){End("motion unavailable");return;}
                Log("slide",StatusText(),anchor,normal);
            } else {
                End("traversal time limit");return;
            }
        } else if(!Play(false)) {
            End("motion unavailable");return;
        }
        viewMode=int(state.mode);viewTicks=state.ticks;status=StatusText();
    }
};
