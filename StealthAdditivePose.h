#pragma once
#include "GunAimMath.h"
#include <vector>
namespace StealthAdditivePose {
struct Node {int bone=-1,parent=-1;D3DXMATRIX base,reference,aim;};
// Retarget the posed rotations onto the active costume's actual local offsets
// and scales. Extra skinning/helper bones keep their complete native local.
inline bool FitToNative(const std::vector<Node>& nodes,const std::vector<D3DXMATRIX>& native,
    const std::vector<D3DXMATRIX>& posed,size_t authoredCount,std::vector<D3DXMATRIX>& result) {
    if(native.size()!=nodes.size() || posed.size()!=nodes.size() || authoredCount>nodes.size())return false;
    result=native;std::vector<unsigned char> done(nodes.size(),0);size_t remaining=nodes.size();
    for(size_t pass=0;pass<nodes.size() && remaining;++pass) {
        bool progress=false;
        for(size_t i=0;i<nodes.size();++i) {
            if(done[i])continue;
            const int p=nodes[i].parent;if(p>=int(nodes.size()) || p==int(i))return false;
            if(p>=0 && !done[p])continue;
            D3DXMATRIX local=native[i],desired=posed[i],inverse;
            if(p>=0) {
                if(!D3DXMatrixInverse(&inverse,nullptr,&native[p]))return false;
                D3DXMatrixMultiply(&local,&native[i],&inverse);
                if(i<authoredCount) {
                    if(!D3DXMatrixInverse(&inverse,nullptr,&posed[p]))return false;
                    D3DXMatrixMultiply(&desired,&posed[i],&inverse);
                }
            }
            if(i<authoredCount) {
                D3DXVECTOR3 scale,position,unusedScale,desiredPosition;D3DXQUATERNION unusedRotation,rotation;
                if(FAILED(D3DXMatrixDecompose(&scale,&unusedRotation,&position,&local)) ||
                    FAILED(D3DXMatrixDecompose(&unusedScale,&rotation,&desiredPosition,&desired)))return false;
                if(p<0)position=desiredPosition; // Keep authored pelvis height and crawl root.
                D3DXQuaternionNormalize(&rotation,&rotation);
                D3DXMatrixTransformation(&local,nullptr,nullptr,&scale,nullptr,&rotation,&position);
            }
            if(p>=0)D3DXMatrixMultiply(&result[i],&local,&result[p]);else result[i]=local;
            done[i]=1;--remaining;progress=true;
        }
        if(!progress)break;
    }
    return remaining==0;
}
inline bool Upper(int bone) {return bone>=1 && bone<=13;}
// Row-vector local rotations: base * inverse(reference) * aim. Translation
// stays on the locomotion skeleton; all children inherit their posed parent.
inline bool Apply(const std::vector<Node>& nodes,float weight,std::vector<D3DXMATRIX>& result) {
    result.clear();for(const auto& n:nodes)result.push_back(n.base);
    weight=(std::clamp)(weight,0.0f,1.0f);if(weight<=0)return true;
    std::vector<unsigned char> done(nodes.size(),0),changed(nodes.size(),0);
    size_t remaining=nodes.size();
    for(size_t pass=0;pass<nodes.size() && remaining;++pass) {
        bool progress=false;
        for(size_t i=0;i<nodes.size();++i) {
            if(done[i])continue;
            const auto& n=nodes[i];const int p=n.parent;
            if(p>=int(nodes.size()) || p==int(i))return false;
            if(p>=0 && !done[p])continue;
            done[i]=1;--remaining;progress=true;
            if(p<0)continue;
            const bool upper=Upper(n.bone);changed[i]=upper||changed[p];
            if(!changed[i])continue; // Preserve pelvis/legs bit-for-bit.
            D3DXMATRIX inverse,local;
            if(!D3DXMatrixInverse(&inverse,nullptr,&nodes[p].base))return false;
            D3DXMatrixMultiply(&local,&n.base,&inverse);
            if(upper) {
                D3DXMATRIX ref,aim,delta,blended,identity;
                if(!D3DXMatrixInverse(&inverse,nullptr,&nodes[p].reference))return false;
                D3DXMatrixMultiply(&ref,&n.reference,&inverse);
                if(!D3DXMatrixInverse(&inverse,nullptr,&nodes[p].aim))return false;
                D3DXMatrixMultiply(&aim,&n.aim,&inverse);
                ref._41=ref._42=ref._43=0;aim._41=aim._42=aim._43=0;
                if(!D3DXMatrixInverse(&inverse,nullptr,&ref))return false;
                D3DXMatrixMultiply(&delta,&inverse,&aim);D3DXMatrixIdentity(&identity);
                if(!GunAimMath::BlendMatrix(identity,delta,weight,blended))return false;
                const float x=local._41,y=local._42,z=local._43;
                local._41=local._42=local._43=0;
                D3DXMatrixMultiply(&delta,&local,&blended);local=delta;
                local._41=x;local._42=y;local._43=z;
            }
            D3DXMatrixMultiply(&result[i],&local,&result[p]);
        }
        if(!progress)break;
    }
    return remaining==0;
}
// Native launcher/grenade aim supplies absolute arm/head orientations each
// frame. Rebuild their positions on the low torso using native bone lengths;
// do not add a fixed rifle grip or alter the native release timing/pitch.
inline bool ApplyNativeArms(const std::vector<Node>& nodes,const std::vector<D3DXMATRIX>& native,std::vector<D3DXMATRIX>& result) {
    if(nodes.size()!=native.size())return false;
    result.clear();for(const auto& n:nodes)result.push_back(n.base);
    std::vector<unsigned char> done(nodes.size(),0),changed(nodes.size(),0);size_t remaining=nodes.size();
    for(size_t pass=0;pass<nodes.size() && remaining;++pass) {
        bool progress=false;
        for(size_t i=0;i<nodes.size();++i) {
            if(done[i])continue;
            const int p=nodes[i].parent;if(p>=int(nodes.size()) || p==int(i))return false;
            if(p>=0 && !done[p])continue;
            done[i]=1;--remaining;progress=true;if(p<0)continue;
            const bool arm=nodes[i].bone==5 || (nodes[i].bone>=6 && nodes[i].bone<=13);
            changed[i]=arm || changed[p];if(!changed[i])continue;
            D3DXMATRIX inverse,local;
            if(!D3DXMatrixInverse(&inverse,nullptr,&nodes[p].base))return false;
            D3DXMatrixMultiply(&local,&nodes[i].base,&inverse);
            D3DXMatrixMultiply(&result[i],&local,&result[p]);
            if(arm)
                for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)
                    result[i].m[row][column]=native[i].m[row][column];
        }
        if(!progress)break;
    }
    return remaining==0;
}

}
