#pragma once
#include <d3dx9math.h>
#include <algorithm>
#include <cmath>
#include "SmgPolicy.h"

namespace GunAimMath
{
    inline float Wrap(float angle) {return std::atan2(std::sin(angle),std::cos(angle));}
    inline float FollowAngle(float current,float target,float maxStep,float blend=.25f)
    {return Wrap(current+(std::clamp)(Wrap(target-current)*blend,-maxStep,maxStep));}
    inline D3DXMATRIX RifleGrip(const D3DXMATRIX& wrist)
    {
        // wp0040_0001: Rx(pi/2)*Rz(-pi/2), with native authored translation.
        D3DXMATRIX x,z,grip,result;
        D3DXMatrixRotationX(&x,D3DX_PI/2);D3DXMatrixRotationZ(&z,-D3DX_PI/2);
        D3DXMatrixMultiply(&grip,&x,&z);grip._41=-.097f;grip._42=-.028f;grip._43=.077f;
        D3DXMatrixMultiply(&result,&grip,&wrist);return result;
    }
    inline bool CameraPoint(const D3DXVECTOR3& camera,const D3DXVECTOR3& ray,
                            const D3DXVECTOR3& player,D3DXVECTOR3& point)
    {
        const float length=D3DXVec3LengthSq(&ray);
        if(!std::isfinite(length) || length<.000001f ||
           !std::isfinite(camera.x+camera.y+camera.z+player.x+player.y+player.z)) return false;
        D3DXVECTOR3 direction;D3DXVec3Normalize(&direction,&ray);
        const auto offset=player-camera;
        // Put the convergence plane a full firing range beyond the player,
        // accounting for the third-person camera's position behind him.
        const float distance=(std::max)(0.0f,D3DXVec3Dot(&offset,&direction))+SmgPolicy::Range;
        point=camera+direction*distance;return true;
    }
    inline float YawCorrection(const D3DXMATRIX& barrel,const D3DXVECTOR3& target)
    {
        const D3DXVECTOR3 direction(target.x-barrel._41-barrel._21*SmgPolicy::MuzzleUp-barrel._31*SmgPolicy::MuzzleForward,
            target.y-barrel._42-barrel._22*SmgPolicy::MuzzleUp-barrel._32*SmgPolicy::MuzzleForward,
            target.z-barrel._43-barrel._23*SmgPolicy::MuzzleUp-barrel._33*SmgPolicy::MuzzleForward);
        if(direction.x*direction.x+direction.z*direction.z<.000001f) return 0;
        const float delta=std::atan2(direction.x,direction.z)-std::atan2(barrel._31,barrel._33);
        return std::atan2(std::sin(delta),std::cos(delta));
    }
    inline bool BlendMatrix(const D3DXMATRIX& a,const D3DXMATRIX& b,float t,D3DXMATRIX& result)
    {
        D3DXVECTOR3 sa,sb,pa,pb;D3DXQUATERNION qa,qb,q;
        if(FAILED(D3DXMatrixDecompose(&sa,&qa,&pa,&a)) || FAILED(D3DXMatrixDecompose(&sb,&qb,&pb,&b))) return false;
        D3DXQuaternionSlerp(&q,&qa,&qb,(std::clamp)(t,0.0f,1.0f));
        D3DXVECTOR3 scale=sa+(sb-sa)*t,position=pa+(pb-pa)*t;
        D3DXMatrixTransformation(&result,nullptr,nullptr,&scale,nullptr,&q,&position);
        return std::isfinite(result._11) && std::isfinite(result._41) && std::isfinite(result._42) && std::isfinite(result._43);
    }
    // Native 616680 skips rotation rebuild for 4002 and matrix rebuild for
    // 8004. A visual Euler pose must release both cached-world states.
    inline unsigned short PoseFlags(unsigned short flags)
    {return static_cast<unsigned short>((flags & ~0xC006u) | 0x10u);}
    inline bool LocalQuaternion(const D3DXMATRIX& world,const D3DXMATRIX& parent,D3DXQUATERNION& quaternion)
    {
        D3DXMATRIX inverse,local;
        if(!D3DXMatrixInverse(&inverse,nullptr,&parent)) return false;
        D3DXMatrixMultiply(&local,&world,&inverse);
        D3DXVECTOR3 scale,position;
        if(FAILED(D3DXMatrixDecompose(&scale,&quaternion,&position,&local))) return false;
        const float length=D3DXQuaternionLengthSq(&quaternion);
        if(!std::isfinite(length) || length<0.000001f) return false;
        D3DXQuaternionNormalize(&quaternion,&quaternion);return true;
    }
    inline D3DXMATRIX MuzzleLocalTransform()
    {
        D3DXMATRIX local;D3DXMatrixIdentity(&local);
        local._42=SmgPolicy::MuzzleUp;local._43=SmgPolicy::MuzzleForward;
        return local;
    }
    inline D3DXMATRIX MuzzleTransform(const D3DXMATRIX& barrel)
    {
        const auto local=MuzzleLocalTransform();D3DXMATRIX world;
        D3DXMatrixMultiply(&world,&local,&barrel);return world;
    }
    inline D3DXVECTOR3 Muzzle(const D3DXMATRIX& m)
    {
        return {m._41+m._21*SmgPolicy::MuzzleUp+m._31*SmgPolicy::MuzzleForward,
                m._42+m._22*SmgPolicy::MuzzleUp+m._32*SmgPolicy::MuzzleForward,
                m._43+m._23*SmgPolicy::MuzzleUp+m._33*SmgPolicy::MuzzleForward};
    }
    inline D3DXVECTOR3 SprayTarget(const D3DXMATRIX& m,float across,float vertical)
    {
        const auto origin=Muzzle(m);
        across=(std::clamp)(across,-1.0f,1.0f)*SmgPolicy::SprayWidth;
        vertical=(std::clamp)(vertical,-1.0f,1.0f)*SmgPolicy::SprayHeight;
        return {origin.x+SmgPolicy::Range*(m._31+m._11*across+m._21*vertical),
                origin.y+SmgPolicy::Range*(m._32+m._12*across+m._22*vertical),
                origin.z+SmgPolicy::Range*(m._33+m._13*across+m._23*vertical)};
    }
    inline bool Step(const D3DXMATRIX& gun,const D3DXVECTOR3& target,float& budget,D3DXMATRIX& rotation)
    {
        D3DXMatrixIdentity(&rotation);
        D3DXVECTOR3 f(gun._31,gun._32,gun._33),d=target-Muzzle(gun);
        if(!std::isfinite(D3DXVec3LengthSq(&d)) || !std::isfinite(D3DXVec3LengthSq(&f)) ||
           D3DXVec3LengthSq(&f)<0.000001f || D3DXVec3LengthSq(&d)<0.000001f) return false;
        D3DXVec3Normalize(&f,&f);D3DXVec3Normalize(&d,&d);
        const float dot=(std::clamp)(D3DXVec3Dot(&f,&d),-1.0f,1.0f);
        if(dot>0.999999f || budget<=0) return true;
        D3DXVECTOR3 axis;D3DXVec3Cross(&axis,&f,&d);
        if(D3DXVec3LengthSq(&axis)<0.000001f)
        {
            const D3DXVECTOR3 basis=std::abs(f.y)>.9f?D3DXVECTOR3(1,0,0):D3DXVECTOR3(0,1,0);
            D3DXVec3Cross(&axis,&f,&basis);
        }
        D3DXVec3Normalize(&axis,&axis);
        const float angle=(std::min)(budget,std::acos(dot));budget-=angle;
        D3DXMatrixRotationAxis(&rotation,&axis,angle);
        return true;
    }
    inline D3DXMATRIX Around(const D3DXMATRIX& source,const D3DXMATRIX& rotation,const D3DXVECTOR3& pivot)
    {
        D3DXMATRIX local=source,result;
        local._41-=pivot.x;local._42-=pivot.y;local._43-=pivot.z;
        D3DXMatrixMultiply(&result,&local,&rotation);
        result._41+=pivot.x;result._42+=pivot.y;result._43+=pivot.z;
        return result;
    }
}
