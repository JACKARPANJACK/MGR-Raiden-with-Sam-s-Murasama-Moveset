#pragma once
#include <d3dx9math.h>
#include <algorithm>
#include <cmath>

namespace PistolIK {
inline bool Unit(D3DXVECTOR3& v) {
    const float length=D3DXVec3Length(&v);
    if(!std::isfinite(length) || length<1e-6f)return false;
    v/=length;return true;
}
struct Chain {D3DXVECTOR3 elbow,hand;float residual=0;};
inline bool Solve(const D3DXVECTOR3& shoulder,const D3DXVECTOR3& elbow,const D3DXVECTOR3& wrist,
    const D3DXVECTOR3& target,const D3DXVECTOR3& pole,Chain& out) {
    auto upper=elbow-shoulder,lower=wrist-elbow,ray=target-shoulder;
    const float a=D3DXVec3Length(&upper),b=D3DXVec3Length(&lower),distance=D3DXVec3Length(&ray);
    if(!std::isfinite(a+b+distance) || a<1e-5f || b<1e-5f || !Unit(ray))return false;
    const float d=(std::clamp)(distance,std::abs(a-b)+1e-5f,a+b-1e-5f);
    auto bend=pole-ray*D3DXVec3Dot(&pole,&ray);
    if(!Unit(bend)) {bend=D3DXVECTOR3(0,-1,0)-ray*(-ray.y);if(!Unit(bend)) {bend=D3DXVECTOR3(1,0,0)-ray*ray.x;if(!Unit(bend))return false;}}
    const float along=(a*a-b*b+d*d)/(2*d),height=std::sqrt((std::max)(0.0f,a*a-along*along));
    out.elbow=shoulder+ray*along+bend*height;out.hand=shoulder+ray*d;
    out.residual=std::abs(distance-d);return true;
}
inline bool Swing(D3DXVECTOR3 from,D3DXVECTOR3 to,D3DXMATRIX& turn) {
    D3DXMatrixIdentity(&turn);if(!Unit(from) || !Unit(to))return false;
    const float dot=(std::clamp)(D3DXVec3Dot(&from,&to),-1.0f,1.0f);
    if(dot>.999999f)return true;
    D3DXVECTOR3 axis;D3DXVec3Cross(&axis,&from,&to);
    if(!Unit(axis)) {D3DXVECTOR3 other=std::abs(from.y)<.9f?D3DXVECTOR3(0,1,0):D3DXVECTOR3(1,0,0);D3DXVec3Cross(&axis,&from,&other);if(!Unit(axis))return false;}
    D3DXMatrixRotationAxis(&turn,&axis,std::acos(dot));return true;
}
}
