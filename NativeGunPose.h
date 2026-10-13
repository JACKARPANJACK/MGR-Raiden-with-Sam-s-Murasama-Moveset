#pragma once
#include <cParts.h>
#include "GunAimMath.h"
#include <vector>
#include <cstring>
#include <cmath>

// Authored world matrices are visual only. Native locals, physics and input
// remain untouched, and all matrices/flags are restored before the next tick.
class NativeGunPose
{
    struct Saved {cParts* part; short bone; Hw::cMtx matrix; unsigned short flags;};
    std::vector<Saved> saved;
    struct LowerSample {short bone;D3DXMATRIX last,from;};
    std::vector<LowerSample> lowerSamples;
    const void* lowerClip=nullptr;
    unsigned lowerBlend=8;
    D3DXMATRIX bodyBasis;
    float facing=0,pitch=0,armYaw=0,recoil=0;
    bool aiming=false;
    cParts* gunPart=nullptr;
    EntityHandle poseOwner,poseGun;
    Hw::cMtx gunMatrix;
    unsigned short gunFlags=0;
    static bool Descendant(cParts* part,cParts* root)
    {
        if(!root) return false;
        for(unsigned depth=0;part && depth<128;++depth,part=part->m_pPartsParent)
            if(part==root) return true;
        return false;
    }
    static bool Upper(int bone)
    {
        return (bone>=1 && bone<=13) || (bone>=256 && bone<=530) ||
            (bone>=1280 && bone<=1296) || bone==1792 || bone==1793 || bone==1808;
    }
    static bool Lower(int bone)
    {return bone==0 || (bone>=15 && bone<=22) || (bone>=1297 && bone<=1302) || bone==1794 || bone==1795;}
    static void Refresh(Pl0000* player)
    {
        (player->m_pParent?player->m_pParent:player)->updateHierarchicalTransformations();
        if(player->m_ppConstraints)
            reinterpret_cast<void(__thiscall*)(void*)>(shared::base+0x69CCB0)(player->m_ppConstraints);
    }
public:
    void BeginFrame()
    {
        Entity* actor=poseOwner.getEntity();
        for(auto& s:saved)
            if(actor && actor->m_pBehavior && actor->m_pBehavior->getPartsPtr(s.bone)==s.part)
            {s.part->m_LocalMatrix=s.matrix;s.part->m_PartsFlag.m_Flag=s.flags;}
        saved.clear();
        Entity* weapon=poseGun.getEntity();
        if(gunPart && weapon && weapon->m_pBehavior && weapon->m_pBehavior->getPartsPtr(0)==gunPart)
        {gunPart->m_LocalMatrix=gunMatrix;gunPart->m_PartsFlag.m_Flag=gunFlags;}
        gunPart=nullptr;poseOwner=EntityHandle{};poseGun=EntityHandle{};
    }
    void Reset() {BeginFrame();aiming=false;pitch=armYaw=recoil=0;lowerClip=nullptr;lowerSamples.clear();}
    void KickRecoil(float amount=0.10f) {recoil=(std::max)(recoil,amount);}
    float Facing() const {return facing;}
    void Prepare(Pl0000* player,const void* reference,const Hw::cVec4& target,float followStep=0.12f)
    {
        cParts* pelvis=player?player->getPartsPtr(0):nullptr;if(!pelvis || !reference) return;
        poseOwner=player->m_pEntity;
        const auto position=pelvis->m_LocalMatrix.r[3];
        const float nativeYaw=player->m_Rot.y;
        const float desired=nativeYaw+(std::clamp)(GunAimMath::Wrap(
            std::atan2(target.x-position.x,target.z-position.z)-nativeYaw),-.610865f,.610865f);
        if(!aiming) {facing=desired;pitch=armYaw=0;aiming=true;}
        else facing=GunAimMath::FollowAngle(facing,desired,followStep);
        // Native locomotion owns the pelvis. Never twist the chest behind it,
        // including when a running turn changes heading faster than aim follows.
        facing=nativeYaw+(std::clamp)(GunAimMath::Wrap(facing-nativeYaw),-.610865f,.610865f);
        D3DXMatrixRotationY(&bodyBasis,facing);
        bodyBasis._41=position.x;bodyBasis._42=position.y;bodyBasis._43=position.z;
        // Capture all native matrices before either body half is overlaid.
        const auto* blob=static_cast<const unsigned char*>(reference);
        unsigned offset=0,count=0;std::memcpy(&offset,blob+12,4);std::memcpy(&count,blob+16,4);
        if(offset!=44 || count>1024) return;
        for(unsigned i=0;i<count;++i)
        {
            short bone=0;std::memcpy(&bone,blob+offset+i*12,2);
            if(!Upper(bone) && !Lower(bone)) continue;
            if(auto* part=player->getPartsPtr(bone)) saved.push_back({part,bone,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
        }
    }
    void Apply(Pl0000* player,const void* data,float frame,bool upper)
    {
        if(!player || !data) return;
        const D3DXMATRIX& basis=bodyBasis;
        if(!upper)
        {
            if(lowerClip!=data)
            {
                lowerClip=data;lowerBlend=0;
                for(auto& sample:lowerSamples) sample.from=sample.last;
            }
            lowerBlend=(std::min)(8u,lowerBlend+1);
        }
        const auto* blob=static_cast<const unsigned char*>(data);
        unsigned offset=0,count=0;short frames=0;
        std::memcpy(&offset,blob+12,4);std::memcpy(&count,blob+16,4);std::memcpy(&frames,blob+10,2);
        if(frames<=0 || count>1024 || offset!=44) return;
        frame=(std::max)(0.0f,(std::min)(float(frames-1),frame));
        const unsigned lo=unsigned(frame),hi=(std::min)(lo+1,unsigned(frames-1));
        for(unsigned i=0;i<count;++i)
        {
            const auto* record=blob+offset+i*12;short bone=0;unsigned keys=0,curve=0;
            std::memcpy(&bone,record,2);std::memcpy(&keys,record+4,4);std::memcpy(&curve,record+8,4);
            if(keys!=unsigned(frames) || !(upper?Upper(bone):Lower(bone))) continue;
            cParts* part=player->getPartsPtr(bone);if(!part) continue;
            bool captured=false;for(auto& s:saved) if(s.part==part) {captured=true;break;}
            if(!captured) saved.push_back({part,bone,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            D3DXMATRIX a,b,relative,world;
            std::memcpy(&a,record+curve+lo*64,64);std::memcpy(&b,record+curve+hi*64,64);
            if(!GunAimMath::BlendMatrix(a,b,frame-lo,relative)) continue;
            if(!upper)
            {
                auto found=std::find_if(lowerSamples.begin(),lowerSamples.end(),[bone](const auto& sample){return sample.bone==bone;});
                if(found==lowerSamples.end()) lowerSamples.push_back({bone,relative,relative});
                else
                {
                    D3DXMATRIX blended;
                    if(lowerBlend<8 && GunAimMath::BlendMatrix(found->from,relative,float(lowerBlend)/8,blended)) relative=blended;
                    found->last=relative;
                }
            }
            D3DXMatrixMultiply(&world,&relative,&basis);
            std::memcpy(&part->m_LocalMatrix,&world,sizeof(world));
            // 616680 preserves a world-fixed bone; the render pipeline uses
            // this authored matrix instead of reinterpreting its local tracks.
            part->m_PartsFlag.m_Flag|=4;
        }
    }
    bool Aim(Pl0000* player,Behavior* gun,const Hw::cVec4& target,float /*budget*/,float followStep=0.075f)
    {
        auto* wrist=player?player->getPartsPtr(9):nullptr;
        auto* left=player?player->getPartsPtr(6):nullptr;
        auto* right=player?player->getPartsPtr(10):nullptr;
        auto* head=player?player->getPartsPtr(5):nullptr;
        auto* muzzle=gun?gun->getPartsPtr(0):nullptr;
        if(!wrist || !left || !right || !head || !muzzle || saved.empty()) return false;
        const D3DXVECTOR3 goal(target.x,target.y,target.z);
        if(!std::isfinite(goal.x+goal.y+goal.z)) return false;
        D3DXMATRIX hand;std::memcpy(&hand,&wrist->m_LocalMatrix,sizeof(hand));
        const auto reference=GunAimMath::RifleGrip(hand);
        const auto direction=goal-GunAimMath::Muzzle(reference);
        const float horizontal=std::hypot(direction.x,direction.z);
        const float desiredPitch=(std::clamp)(std::atan2(direction.y,horizontal)+recoil,-D3DX_PI*.305556f,D3DX_PI*.305556f);
        pitch=GunAimMath::FollowAngle(pitch,desiredPitch,followStep);
        recoil*=0.68f;if(recoil<0.001f) recoil=0;
        const float desiredArm=(std::clamp)(GunAimMath::Wrap(std::atan2(direction.x,direction.z)-facing),-.436332f,.436332f);
        armYaw=GunAimMath::FollowAngle(armYaw,desiredArm,followStep);
        const auto l=left->m_LocalMatrix.r[3],r=right->m_LocalMatrix.r[3];
        const D3DXVECTOR3 pivot((l.x+r.x)*.5f,(l.y+r.y)*.5f,(l.z+r.z)*.5f);
        const D3DXVECTOR3 axis(bodyBasis._11,bodyBasis._12,bodyBasis._13);
        D3DXMATRIX pitchTurn,yawTurn,turn;
        D3DXMatrixRotationAxis(&pitchTurn,&axis,-pitch);D3DXMatrixRotationY(&yawTurn,armYaw);
        D3DXMatrixMultiply(&turn,&pitchTurn,&yawTurn);
        for(auto& s:saved)
        {
            if(!Descendant(s.part,left) && !Descendant(s.part,right)) continue;
            D3DXMATRIX source;std::memcpy(&source,&s.part->m_LocalMatrix,sizeof(source));
            const auto corrected=GunAimMath::Around(source,turn,pivot);
            std::memcpy(&s.part->m_LocalMatrix,&corrected,sizeof(corrected));
        }
        // Keep the head upright. Gun pitch acts on the arm chains, never the
        // neck/head or the entire chest. Hair/face children move with the head.
        D3DXMATRIX headWorld,headInverse,neutral;
        std::memcpy(&headWorld,&head->m_LocalMatrix,sizeof(headWorld));
        D3DXMatrixRotationY(&neutral,facing+armYaw*.25f);
        neutral._41=headWorld._41;neutral._42=headWorld._42;neutral._43=headWorld._43;
        if(D3DXMatrixInverse(&headInverse,nullptr,&headWorld))
            for(auto& s:saved) if(Descendant(s.part,head))
            {
                D3DXMATRIX source,local,world;std::memcpy(&source,&s.part->m_LocalMatrix,sizeof(source));
                D3DXMatrixMultiply(&local,&source,&headInverse);D3DXMatrixMultiply(&world,&local,&neutral);
                std::memcpy(&s.part->m_LocalMatrix,&world,sizeof(world));
            }
        Refresh(player);
        // Solve from this frame's wrist, not the weapon's previous update.
        // This also makes rendering, muzzle flash and bullet origin agree.
        std::memcpy(&hand,&wrist->m_LocalMatrix,sizeof(hand));
        const auto barrel=GunAimMath::RifleGrip(hand);
        poseGun=gun->m_pEntity;
        gunPart=muzzle;gunMatrix=muzzle->m_LocalMatrix;gunFlags=muzzle->m_PartsFlag.m_Flag;
        std::memcpy(&muzzle->m_LocalMatrix,&barrel,sizeof(barrel));muzzle->m_PartsFlag.m_Flag|=4;
        (gun->m_pParent?gun->m_pParent:gun)->updateHierarchicalTransformations();
        auto aimed=goal-GunAimMath::Muzzle(barrel);D3DXVECTOR3 forward(barrel._31,barrel._32,barrel._33);
        if(D3DXVec3LengthSq(&aimed)<.000001f) return false;
        D3DXVec3Normalize(&aimed,&aimed);D3DXVec3Normalize(&forward,&forward);
        return D3DXVec3Dot(&aimed,&forward)>.995f;
    }
};
