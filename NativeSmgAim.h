#pragma once
#include <cParts.h>
#include "GunAimMath.h"
#include <d3dx9math.h>
#include <algorithm>
#include <cmath>
#include <cstring>

// Rotate the authored two-handed cyborg pose as one upper-body chain.
class NativeSmgAim
{
    Pl0000* owner=nullptr;
    cParts* chest=nullptr;
    Hw::cVec4 savedRotation;
    Hw::cQuat savedQuaternion;
    unsigned short savedFlags=0;
    bool applied=false;
    void Refresh()
    {
        (owner->m_pParent?owner->m_pParent:owner)->updateHierarchicalTransformations();
        if(owner->m_ppConstraints)
            reinterpret_cast<void(__thiscall*)(void*)>(shared::base+0x69CCB0)(owner->m_ppConstraints);
    }
public:
    void BeginFrame()
    {
        if(!applied || !chest) return;
        chest->m_Rot=savedRotation;chest->m_QuatRot=savedQuaternion;
        chest->m_PartsFlag.m_Flag=savedFlags;applied=false;
        Refresh();
    }
    void Reset() {BeginFrame();owner=nullptr;chest=nullptr;}
    bool Bind(Pl0000* player)
    {
        if(player==owner && chest) return true;
        Reset();
        const unsigned char decompose[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x81,0xEC,0xAC,0,0,0};
        const unsigned char constraints[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x81,0xEC,0xA4,0,0,0};
        if(!player ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0xA30580),decompose,sizeof(decompose)) ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0x69CCB0),constraints,sizeof(constraints))) return false;
        chest=player->getPartsPtr(3);
        if(!chest || !chest->m_pPartsParent) {chest=nullptr;return false;}
        owner=player;return true;
    }
    bool Aim(Behavior* gun,const Hw::cVec4& target,float turnLimit=D3DX_PI/3)
    {
        if(!owner || !chest || !gun || !std::isfinite(target.x) ||
           !std::isfinite(target.y) || !std::isfinite(target.z)) return false;
        auto* root=gun->getPartsPtr(0);if(!root) return false;
        Refresh(); // Sample this frame's weapon constraint, not the previous one.
        savedRotation=chest->m_Rot;savedQuaternion=chest->m_QuatRot;
        savedFlags=chest->m_PartsFlag.m_Flag;applied=true;
        float remaining=turnLimit;
        for(unsigned pass=0;pass<3 && remaining>0.0001f;++pass)
        {
            D3DXMATRIX gunMatrix,source,rotation;
            std::memcpy(&gunMatrix,&root->m_LocalMatrix,sizeof(gunMatrix));
            if(!GunAimMath::Step(gunMatrix,D3DXVECTOR3(target.x,target.y,target.z),remaining,rotation))
            {BeginFrame();return false;}
            std::memcpy(&source,&chest->m_LocalMatrix,sizeof(source));
            const auto result=GunAimMath::Around(source,rotation,D3DXVECTOR3(source._41,source._42,source._43));
            std::memcpy(&chest->m_LocalMatrix,&result,sizeof(result));
            D3DXMATRIX parent;std::memcpy(&parent,&chest->m_pPartsParent->m_LocalMatrix,sizeof(parent));
            D3DXQUATERNION local;
            if(!GunAimMath::LocalQuaternion(result,parent,local)) {BeginFrame();return false;}
            std::memcpy(&chest->m_QuatRot,&local,sizeof(local));
            // Keep the quaternion authoritative during propagation. Do not
            // re-decompose it through Euler angles near a singular pitch.
            chest->m_PartsFlag.m_Flag=GunAimMath::PoseFlags(chest->m_PartsFlag.m_Flag)|0x4000;
            Refresh(); // Iterate because rotating around the chest also moves the muzzle.
        }
        const auto& m=root->m_LocalMatrix;
        D3DXVECTOR3 f(m.r[2].x,m.r[2].y,m.r[2].z);
        D3DXVECTOR3 d(target.x-m.r[3].x-m.r[1].x*SmgPolicy::MuzzleUp-m.r[2].x*SmgPolicy::MuzzleForward,
                      target.y-m.r[3].y-m.r[1].y*SmgPolicy::MuzzleUp-m.r[2].y*SmgPolicy::MuzzleForward,
                      target.z-m.r[3].z-m.r[1].z*SmgPolicy::MuzzleUp-m.r[2].z*SmgPolicy::MuzzleForward);
        D3DXVec3Normalize(&f,&f);D3DXVec3Normalize(&d,&d);
        return D3DXVec3Dot(&f,&d)>0.999f;
    }
};
