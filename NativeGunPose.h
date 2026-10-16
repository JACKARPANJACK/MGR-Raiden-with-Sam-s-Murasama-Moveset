#pragma once
#include <cParts.h>
#include "GunAimMath.h"
#include "PistolIK.h"
#include <vector>
#include <cstring>
#include <cmath>

// Authored world matrices are visual only. Native locals, physics and input
// remain untouched, and all matrices/flags are restored before the next tick.
class NativeGunPose
{
    struct Saved {cParts* part; short bone; Hw::cMtx matrix; unsigned short flags;int slot=-1;};
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
        // Weapon/sheath attachment markers (700/701/710) belong to native
        // sword handling, not to the cyborg's authored upper-body pose.
        return (bone>=1 && bone<=13) || (bone>=256 && bone<=530) ||
            (bone>=1280 && bone<=1296);
    }
    static bool Lower(int bone)
    {return bone==0 || (bone>=15 && bone<=22) || (bone>=1297 && bone<=1302) || bone==1794 || bone==1795;}
    static void Refresh(Pl0000* player)
    {
        (player->m_pParent?player->m_pParent:player)->updateHierarchicalTransformations();
        if(player->m_ppConstraints)
            reinterpret_cast<void(__thiscall*)(void*)>(shared::base+0x69CCB0)(player->m_ppConstraints);
    }
    static D3DXVECTOR3 Position(cParts* part) {
        const auto& p=part->m_LocalMatrix.r[3];return {p.x,p.y,p.z};
    }
    void TurnChain(cParts* root,const D3DXMATRIX& turn,const D3DXVECTOR3& pivot) {
        for(auto& s:saved)if(Descendant(s.part,root)) {
            D3DXMATRIX source;std::memcpy(&source,&s.part->m_LocalMatrix,64);
            const auto world=GunAimMath::Around(source,turn,pivot);
            std::memcpy(&s.part->m_LocalMatrix,&world,64);s.part->m_PartsFlag.m_Flag|=4;
        }
    }
    bool SolvePistolArm(Pl0000* player,int upper,int fore,int hand,const D3DXVECTOR3& target,
        const D3DXVECTOR3& pole,const D3DXMATRIX& wristGoal) {
        auto* shoulder=player->getPartsPtr(short(upper));auto* elbow=player->getPartsPtr(short(fore));auto* wrist=player->getPartsPtr(short(hand));
        if(!shoulder || !elbow || !wrist)return false;
        PistolIK::Chain solve;
        if(!PistolIK::Solve(Position(shoulder),Position(elbow),Position(wrist),target,pole,solve))return false;
        D3DXMATRIX swing;
        if(!PistolIK::Swing(Position(elbow)-Position(shoulder),solve.elbow-Position(shoulder),swing))return false;
        TurnChain(shoulder,swing,Position(shoulder));
        if(!PistolIK::Swing(Position(wrist)-Position(elbow),solve.hand-Position(elbow),swing))return false;
        TurnChain(elbow,swing,Position(elbow));
        D3DXMATRIX current,goal,inverse,correction;
        std::memcpy(&current,&wrist->m_LocalMatrix,64);goal=wristGoal;
        current._41=current._42=current._43=0;goal._41=goal._42=goal._43=0;
        if(!D3DXMatrixInverse(&inverse,nullptr,&current))return false;
        D3DXMatrixMultiply(&correction,&inverse,&goal);TurnChain(wrist,correction,Position(wrist));
        return true;
    }
    bool AimPistol(Pl0000* player,Behavior* gun,const Hw::cVec4& target,float followStep,
        const D3DXMATRIX& grip,float muzzleUp,float muzzleForward) {
        auto* right=player->getPartsPtr(11);auto* left=player->getPartsPtr(7);
        auto* wrist=player->getPartsPtr(13);auto* support=player->getPartsPtr(9);auto* muzzle=gun->getPartsPtr(0);
        auto* rightElbow=player->getPartsPtr(12);auto* leftElbow=player->getPartsPtr(8);
        if(!right || !left || !wrist || !support || !muzzle || !rightElbow || !leftElbow || saved.empty())return false;
        D3DXMATRIX hand,other,reference;std::memcpy(&hand,&wrist->m_LocalMatrix,64);std::memcpy(&other,&support->m_LocalMatrix,64);
        D3DXMatrixMultiply(&reference,&grip,&hand);
        const D3DXVECTOR3 goal(target.x,target.y,target.z),pivot=(Position(right)+Position(left))*.5f;
        const auto direction=goal-Position(wrist);
        if(!std::isfinite(direction.x+direction.y+direction.z))return false;
        pitch=GunAimMath::FollowAngle(pitch,(std::clamp)(std::atan2(direction.y,std::hypot(direction.x,direction.z))+recoil,-1.15f,1.15f),followStep);
        armYaw=GunAimMath::FollowAngle(armYaw,(std::clamp)(GunAimMath::Wrap(std::atan2(direction.x,direction.z)-facing),-.65f,.65f),followStep);
        recoil*=.68f;
        D3DXMATRIX desired,referenceRotation=reference,inverse,turn;
        D3DXMatrixRotationYawPitchRoll(&desired,facing+armYaw,-pitch,0);
        referenceRotation._41=referenceRotation._42=referenceRotation._43=0;
        D3DXVECTOR3 scale,position;D3DXQUATERNION rotation;
        if(FAILED(D3DXMatrixDecompose(&scale,&rotation,&position,&referenceRotation)))return false;
        D3DXMatrixRotationQuaternion(&referenceRotation,&rotation);
        if(!D3DXMatrixInverse(&inverse,nullptr,&referenceRotation))return false;
        D3DXMatrixMultiply(&turn,&inverse,&desired);
        const auto rotated=GunAimMath::Around(hand,turn,pivot);
        const auto otherRotated=GunAimMath::Around(other,turn,pivot);
        auto poleR=Position(rightElbow)-Position(right),poleL=Position(leftElbow)-Position(left);
        D3DXVec3TransformNormal(&poleR,&poleR,&turn);D3DXVec3TransformNormal(&poleL,&poleL,&turn);
        const D3DXVECTOR3 rightGoal(rotated._41,rotated._42,rotated._43);
        if(!SolvePistolArm(player,11,12,13,rightGoal,poleR,rotated))return false;
        // Anchor the support hand to the actual solved firing hand. Costume
        // proportions may clamp its reach, but never separate the two grips.
        const D3DXVECTOR3 gap(otherRotated._41-rotated._41,otherRotated._42-rotated._42,otherRotated._43-rotated._43);
        if(!SolvePistolArm(player,7,8,9,Position(wrist)+gap,poleL,otherRotated))return false;
        Refresh(player);std::memcpy(&hand,&wrist->m_LocalMatrix,64);
        D3DXMATRIX barrel;D3DXMatrixMultiply(&barrel,&grip,&hand);
        // Refit to the active costume's palm landmarks after IK. This avoids
        // assuming every costume has the reference rig's wrist/finger offsets.
        D3DXVECTOR3 knuckles(0,0,0);unsigned fingers=0;
        for(short id:{short(516),short(520),short(524),short(528)})if(auto* finger=player->getPartsPtr(id)) {knuckles+=Position(finger);++fingers;}
        if(fingers==4) {
            const auto palm=(Position(wrist)+knuckles*.25f)*.5f;
            const D3DXVECTOR3 contact(PistolHandling::GripX*PistolHandling::ModelScale,
                PistolHandling::GripY*PistolHandling::ModelScale,PistolHandling::GripZ*PistolHandling::ModelScale);
            D3DXVECTOR3 offset;D3DXVec3TransformNormal(&offset,&contact,&barrel);
            barrel._41=palm.x-offset.x;barrel._42=palm.y-offset.y;barrel._43=palm.z-offset.z;
        }
        if(!gunPart) {poseGun=gun->m_pEntity;gunPart=muzzle;gunMatrix=muzzle->m_LocalMatrix;gunFlags=muzzle->m_PartsFlag.m_Flag;}
        std::memcpy(&muzzle->m_LocalMatrix,&barrel,64);muzzle->m_PartsFlag.m_Flag|=4;
        (gun->m_pParent?gun->m_pParent:gun)->updateHierarchicalTransformations();
        D3DXVECTOR3 origin(barrel._41+barrel._21*muzzleUp+barrel._31*muzzleForward,
            barrel._42+barrel._22*muzzleUp+barrel._32*muzzleForward,barrel._43+barrel._23*muzzleUp+barrel._33*muzzleForward);
        auto ray=goal-origin;D3DXVECTOR3 forward(barrel._31,barrel._32,barrel._33);
        return PistolIK::Unit(ray) && PistolIK::Unit(forward) && D3DXVec3Dot(&ray,&forward)>.99f;
    }
public:
    void BeginFrame(bool keepGun=false)
    {
        Entity* actor=poseOwner.getEntity();
        for(auto& s:saved) {
            if(!actor || !actor->m_pBehavior)continue;
            auto* storage=actor->m_pBehavior->m_pParent?actor->m_pBehavior->m_pParent:actor->m_pBehavior;
            const auto& bones=storage->m_PartsData;
            auto* current=s.slot<0?actor->m_pBehavior->getPartsPtr(s.bone):
                s.slot<bones.m_BoneNum?(bones.m_ppBones?bones.m_ppBones[s.slot]:bones.m_pBones?bones.m_pBones+s.slot:nullptr):nullptr;
            if(current==s.part) {s.part->m_LocalMatrix=s.matrix;s.part->m_PartsFlag.m_Flag=s.flags;}
        }
        saved.clear();
        if(!keepGun) {
            Entity* weapon=poseGun.getEntity();
            if(gunPart && weapon && weapon->m_pBehavior && weapon->m_pBehavior->getPartsPtr(0)==gunPart)
            {gunPart->m_LocalMatrix=gunMatrix;gunPart->m_PartsFlag.m_Flag=gunFlags;}
            gunPart=nullptr;poseGun=EntityHandle{};
        }
        poseOwner=EntityHandle{};
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
        // Costumes add jacket, twist and helper bones absent from the donor.
        // Capture their native locals too, even when their matrices are cached.
        auto* storage=player->m_pParent?player->m_pParent:player;
        const auto& bones=storage->m_PartsData;
        if(bones.m_BoneNum>0 && bones.m_BoneNum<=4096)
            for(int i=0;i<bones.m_BoneNum;++i) {
                auto* part=bones.m_ppBones?bones.m_ppBones[i]:bones.m_pBones?bones.m_pBones+i:nullptr;
                if(!part || !Descendant(part,player->getPartsPtr(1)))continue;
                bool captured=false;for(const auto& s:saved)if(s.part==part){captured=true;break;}
                if(!captured)saved.push_back({part,-1,part->m_LocalMatrix,part->m_PartsFlag.m_Flag,i});
            }
    }
    void ApplyUpper(Pl0000* player,const void* data,float frame,bool armsOnly=false)
    {
        const auto* blob=static_cast<const unsigned char*>(data);
        unsigned offset=0,count=0;short frames=0;
        std::memcpy(&offset,blob+12,4);std::memcpy(&count,blob+16,4);std::memcpy(&frames,blob+10,2);
        if(offset!=44 || count>1024 || frames<=0)return;
        frame=(std::clamp)(frame,0.0f,float(frames-1));
        const unsigned lo=unsigned(frame),hi=(std::min)(lo+1,unsigned(frames-1));
        struct Sample {short bone;D3DXMATRIX world;};
        std::vector<Sample> samples;
        for(unsigned i=0;i<count;++i) {
            const auto* record=blob+offset+i*12;short bone;unsigned keys,curve;
            std::memcpy(&bone,record,2);std::memcpy(&keys,record+4,4);std::memcpy(&curve,record+8,4);
            if(keys!=unsigned(frames) || !Upper(bone))continue;
            D3DXMATRIX a,b,relative,world;
            std::memcpy(&a,record+curve+lo*64,64);std::memcpy(&b,record+curve+hi*64,64);
            if(!GunAimMath::BlendMatrix(a,b,frame-lo,relative))continue;
            D3DXMatrixMultiply(&world,&relative,&bodyBasis);samples.push_back({bone,world});
        }
        auto sample=[&](short bone)->const D3DXMATRIX* {
            for(const auto& s:samples)if(s.bone==bone)return &s.world;
            return nullptr;
        };
        std::vector<D3DXMATRIX> result(saved.size());
        std::vector<unsigned char> done(saved.size(),0),changed(saved.size(),0);
        auto* spine=player->getPartsPtr(1);
        for(size_t pass=0;pass<saved.size();++pass) {
            bool progress=false;
            for(size_t i=0;i<saved.size();++i) {
                if(done[i])continue;
                const auto& s=saved[i];auto* parent=s.part->m_pPartsParent;
                size_t p=saved.size();
                for(size_t j=0;j<saved.size();++j)if(saved[j].part==parent){p=j;break;}
                if(p<saved.size() && !done[p])continue;
                done[i]=1;progress=true;std::memcpy(&result[i],&s.matrix,64);
                if(!parent || !Descendant(s.part,spine))continue;
                D3DXMATRIX parentNative,inverse,local;
                std::memcpy(&parentNative,p<saved.size()?&saved[p].matrix:&parent->m_LocalMatrix,64);
                if(!D3DXMatrixInverse(&inverse,nullptr,&parentNative))continue;
                D3DXMatrixMultiply(&local,&result[i],&inverse);
                const auto* authored=sample(s.bone);
                if(authored && (!armsOnly || (s.bone>=6 && s.bone<=13) || (s.bone>=256 && s.bone<=530))) {
                    const auto* parentAuthored=p<saved.size()?sample(saved[p].bone):nullptr;
                    if(parentAuthored && !D3DXMatrixInverse(&inverse,nullptr,parentAuthored))continue;
                    D3DXMATRIX authoredLocal;D3DXMatrixMultiply(&authoredLocal,authored,&inverse);
                    D3DXVECTOR3 scale,position,unusedScale,unusedPosition;D3DXQUATERNION rotation,unusedRotation;
                    if(FAILED(D3DXMatrixDecompose(&scale,&unusedRotation,&position,&local)) ||
                        FAILED(D3DXMatrixDecompose(&unusedScale,&rotation,&unusedPosition,&authoredLocal)))continue;
                    // Transfer rotation only. Absolute donor positions/scales
                    // must never stretch the active costume's skinning rig.
                    D3DXQuaternionNormalize(&rotation,&rotation);
                    D3DXMatrixTransformation(&local,nullptr,nullptr,&scale,nullptr,&rotation,&position);
                }
                D3DXMatrixMultiply(&result[i],&local,p<saved.size()?&result[p]:&parentNative);
                changed[i]=1;
            }
            if(!progress)break;
        }
        for(size_t i=0;i<saved.size();++i)if(changed[i]) {
            std::memcpy(&saved[i].part->m_LocalMatrix,&result[i],64);
            saved[i].part->m_PartsFlag.m_Flag|=4;
        }
    }
    void Apply(Pl0000* player,const void* data,float frame,bool upper,bool armsOnly=false)
    {
        if(!player || !data) return;
        if(upper) {ApplyUpper(player,data,frame,armsOnly);return;}
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
    bool Aim(Pl0000* player,Behavior* gun,const Hw::cVec4& target,float /*budget*/,float followStep=0.075f,bool compact=false,const D3DXMATRIX* pistolGrip=nullptr,float pistolUp=.030f,float pistolForward=.132f)
    {
        if(pistolGrip)return player && gun && AimPistol(player,gun,target,followStep,*pistolGrip,pistolUp,pistolForward);
        // Both guns use the rifle ready pose: its firing grip is left wrist 9.
        // The enemy SMG's right-hand marker is incompatible with this pose.
        auto* wrist=player?player->getPartsPtr(pistolGrip?13:9):nullptr;
        auto* left=player?player->getPartsPtr(7):nullptr;
        auto* right=player?player->getPartsPtr(11):nullptr;
        auto* head=player?player->getPartsPtr(5):nullptr;
        auto* muzzle=gun?gun->getPartsPtr(0):nullptr;
        if(!wrist || !left || !right || !head || !muzzle || saved.empty()) return false;
        const D3DXVECTOR3 goal(target.x,target.y,target.z);
        if(!std::isfinite(goal.x+goal.y+goal.z)) return false;
        D3DXMATRIX hand;std::memcpy(&hand,&wrist->m_LocalMatrix,sizeof(hand));
        // Keep the SMG's own muzzle geometry after aligning its root to the
        // rifle grip. Its marker 0x300 is at (0,.016,.330271).
        const auto origin=[compact,pistolGrip,pistolUp,pistolForward](const D3DXMATRIX& m) {
            if(pistolGrip)return D3DXVECTOR3(m._41+m._21*pistolUp+m._31*pistolForward,
                m._42+m._22*pistolUp+m._32*pistolForward,m._43+m._23*pistolUp+m._33*pistolForward);
            if(!compact)return GunAimMath::Muzzle(m);
            return D3DXVECTOR3(m._41+m._21*.016f+m._31*.330271f,
                m._42+m._22*.016f+m._32*.330271f,
                m._43+m._23*.016f+m._33*.330271f);
        };
        const auto grip=[pistolGrip](const D3DXMATRIX& wrist) {
            if(!pistolGrip)return GunAimMath::RifleGrip(wrist);
            D3DXMATRIX result;D3DXMatrixMultiply(&result,pistolGrip,&wrist);return result;
        };
        const auto reference=grip(hand);
        const auto direction=goal-origin(reference);
        const float horizontal=std::hypot(direction.x,direction.z);
        const float desiredPitch=(std::clamp)(std::atan2(direction.y,horizontal)+recoil,-D3DX_PI*.305556f,D3DX_PI*.305556f);
        pitch=GunAimMath::FollowAngle(pitch,desiredPitch,followStep);
        recoil*=0.68f;if(recoil<0.001f) recoil=0;
        const float desiredArm=(std::clamp)(GunAimMath::Wrap(std::atan2(direction.x,direction.z)-facing),-.436332f,.436332f);
        armYaw=GunAimMath::FollowAngle(armYaw,desiredArm,followStep);
        const auto l=left->m_LocalMatrix.r[3],r=right->m_LocalMatrix.r[3];
        const D3DXVECTOR3 axis(bodyBasis._11,bodyBasis._12,bodyBasis._13);
        D3DXMATRIX pitchTurn,yawTurn,turn;
        D3DXMatrixRotationAxis(&pitchTurn,&axis,-pitch);D3DXMatrixRotationY(&yawTurn,armYaw);
        D3DXMatrixMultiply(&turn,&pitchTurn,&yawTurn);
        for(auto& s:saved)
        {
            if(!Descendant(s.part,left) && !Descendant(s.part,right)) continue;
            D3DXMATRIX source;std::memcpy(&source,&s.part->m_LocalMatrix,sizeof(source));
            const auto& joint=Descendant(s.part,left)?l:r;
            const auto corrected=GunAimMath::Around(source,turn,D3DXVECTOR3(joint.x,joint.y,joint.z));
            std::memcpy(&s.part->m_LocalMatrix,&corrected,sizeof(corrected));
        }
        // Keep the head upright. Gun pitch acts on the arm chains, never the
        // neck/head or the entire chest. Hair/face children move with the head.
        D3DXMATRIX headWorld,headInverse,neutral;
        std::memcpy(&headWorld,&head->m_LocalMatrix,sizeof(headWorld));
        D3DXMatrixRotationY(&neutral,facing+armYaw*.25f);
        D3DXVECTOR3 headScale,headPosition;D3DXQUATERNION headRotation;
        if(SUCCEEDED(D3DXMatrixDecompose(&headScale,&headRotation,&headPosition,&headWorld))) {
            D3DXMATRIX scale,scaled;D3DXMatrixScaling(&scale,headScale.x,headScale.y,headScale.z);
            D3DXMatrixMultiply(&scaled,&scale,&neutral);neutral=scaled;
        }
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
        const auto barrel=grip(hand);
        // Effects update during the native tick. Keep this root world-fixed
        // across body restoration while aiming, rather than exposing the
        // native lowered/leg pose between visual frames. Save it only once;
        // Reset/cancellation restores the original matrix and flags.
        if(!gunPart) {
            poseGun=gun->m_pEntity;
            gunPart=muzzle;gunMatrix=muzzle->m_LocalMatrix;gunFlags=muzzle->m_PartsFlag.m_Flag;
        }
        std::memcpy(&muzzle->m_LocalMatrix,&barrel,sizeof(barrel));muzzle->m_PartsFlag.m_Flag|=4;
        (gun->m_pParent?gun->m_pParent:gun)->updateHierarchicalTransformations();
        auto aimed=goal-origin(barrel);D3DXVECTOR3 forward(barrel._31,barrel._32,barrel._33);
        if(D3DXVec3LengthSq(&aimed)<.000001f) return false;
        D3DXVec3Normalize(&aimed,&aimed);D3DXVec3Normalize(&forward,&forward);
        return D3DXVec3Dot(&aimed,&forward)>.995f;
    }
};
