#pragma once
#include "StealthAdditivePose.h"
#include <vector>
#include <cstring>

// The native MOT/BXM owns action time and sequence events. This final render
// pass owns the complete visible attack, fitted to the costume captured before
// playback. Restore all matrices before the next native update.
class PistolFullBodyPose {
    struct Part {cParts* pointer=nullptr;short bone=-1;int slot=-1;};
    struct Saved {Part part;Hw::cMtx matrix;unsigned short flags=0;};
    EntityHandle owner;
    std::vector<Part> parts;
    std::vector<StealthAdditivePose::Node> nodes;
    std::vector<D3DXMATRIX> native;
    std::vector<Saved> saved;
    std::vector<D3DXMATRIX> last,blendFrom;
    std::vector<D3DXMATRIX> entryPose;
    float blendDuration=4;
    size_t authored=0;
    float bindHeight=0;
    static bool Body(short bone) {
        return (bone>=0 && bone<=22 && bone!=14) || (bone>=256 && bone<=530) ||
            (bone>=1280 && bone<=1302) || bone==1794 || bone==1795;
    }
    static D3DXMATRIX Basis(Pl0000* p) {
        D3DXMATRIX result;D3DXMatrixRotationY(&result,p->m_Rot.y);
        result._41=p->m_TransPos.x;result._42=p->m_TransPos.y;result._43=p->m_TransPos.z;return result;
    }
    bool Valid(Pl0000* p,const Part& part) const {
        if(!p || !p->m_pEntity || owner.getEntity()!=p->m_pEntity)return false;
        if(part.bone>=0)return p->getPartsPtr(part.bone)==part.pointer;
        auto* storage=p->m_pParent?p->m_pParent:p;const auto& b=storage->m_PartsData;
        return part.slot>=0 && part.slot<b.m_BoneNum &&
            (b.m_ppBones?b.m_ppBones[part.slot]:b.m_pBones?b.m_pBones+part.slot:nullptr)==part.pointer;
    }
public:
    bool CaptureEntry(Pl0000* p) {
        D3DXMATRIX inverse,basis=Basis(p);if(!D3DXMatrixInverse(&inverse,nullptr,&basis))return false;
        entryPose.clear();
        for(const auto& part:parts) {
            D3DXMATRIX world,relative;std::memcpy(&world,&part.pointer->m_LocalMatrix,64);
            D3DXMatrixMultiply(&relative,&world,&inverse);entryPose.push_back(relative);
        }
        return true;
    }
    void BeginFrame() {
        auto* entity=owner.getEntity();auto* p=entity&&entity->m_pBehavior?static_cast<Pl0000*>(entity->m_pBehavior):nullptr;
        for(const auto& s:saved)if(Valid(p,s.part)){s.part.pointer->m_LocalMatrix=s.matrix;s.part.pointer->m_PartsFlag.m_Flag=s.flags;}
        saved.clear();
    }
    void End(){BeginFrame();last.clear();blendFrom.clear();entryPose.clear();}
    void BeginStage(float frames=4){blendDuration=(std::max)(1.0f,frames);blendFrom=last.empty()?entryPose:last;}
    void Reset(){End();parts.clear();nodes.clear();native.clear();authored=0;owner=EntityHandle{};}
    bool Capture(Pl0000* p,const void* data) {
        if(!p || !p->m_pEntity || !data || std::memcmp(data,"FBW1",4))return false;
        if(!parts.empty()) {
            bool same=true;for(const auto& part:parts)if(!Valid(p,part)){same=false;break;}
            if(same){BeginFrame();return CaptureEntry(p);} // Preserve bind offsets but capture the actual entry pose.
        }
        Reset();
        owner=p->m_pEntity;
        const auto* blob=static_cast<const unsigned char*>(data);
        std::memcpy(&bindHeight,blob+36,4);
        if(!std::isfinite(bindHeight) || bindHeight<.1f || bindHeight>5)return false;
        unsigned count=0,table=0;std::memcpy(&table,blob+12,4);std::memcpy(&count,blob+16,4);
        if(table!=44 || count>1024)return false;
        for(unsigned i=0;i<count;++i) {
            short bone;std::memcpy(&bone,blob+44+i*12,2);
            if(!Body(bone))continue;
            if(auto* part=p->getPartsPtr(bone)) {
                bool found=false;for(const auto& entry:parts)if(entry.pointer==part){found=true;break;}
                if(!found)parts.push_back({part,bone,-1});
            }
        }
        authored=parts.size();
        for(short bone:{short(0),short(1),short(2),short(3),short(4),short(5),short(6),short(7),short(8),short(9),
            short(10),short(11),short(12),short(13),short(15),short(16),short(17),short(18),short(19),short(20),short(21),short(22)}) {
            bool found=false;for(const auto& entry:parts)if(entry.bone==bone){found=true;break;}
            if(!found)return false;
        }
        auto* storage=p->m_pParent?p->m_pParent:p;const auto& b=storage->m_PartsData;
        if(b.m_BoneNum<=0 || b.m_BoneNum>4096)return false;
        for(int i=0;i<b.m_BoneNum;++i) {
            auto* part=b.m_ppBones?b.m_ppBones[i]:b.m_pBones?b.m_pBones+i:nullptr;
            if(!part)continue;
            bool found=false;for(const auto& entry:parts)if(entry.pointer==part){found=true;break;}
            if(!found)parts.push_back({part,-1,i});
        }
        D3DXMATRIX inverse,basis=Basis(p);if(!D3DXMatrixInverse(&inverse,nullptr,&basis))return false;
        for(const auto& part:parts) {
            StealthAdditivePose::Node node;node.bone=part.bone;
            for(size_t j=0;j<parts.size();++j)if(parts[j].pointer==part.pointer->m_pPartsParent){node.parent=int(j);break;}
            if(node.bone==0)node.parent=-1; // Authored pelvis height is actor-relative.
            D3DXMATRIX world,relative;std::memcpy(&world,&part.pointer->m_LocalMatrix,64);
            D3DXMatrixMultiply(&relative,&world,&inverse);native.push_back(relative);nodes.push_back(node);
        }
        return authored>=22 && CaptureEntry(p);
    }
    bool Apply(Pl0000* p,const void* data,float frame) {
        if(!data || parts.empty() || std::memcmp(data,"FBW1",4))return false;
        for(const auto& part:parts)if(!Valid(p,part))return false;
        const auto* blob=static_cast<const unsigned char*>(data);
        unsigned count=0;unsigned short frames=0;std::memcpy(&count,blob+16,4);std::memcpy(&frames,blob+10,2);
        if(!frames || count>1024 || !std::isfinite(frame))return false;
        frame=(std::clamp)(frame,0.0f,float(frames-1));const unsigned lo=unsigned(frame),hi=(std::min)(lo+1,unsigned(frames-1));
        auto desired=native;
        for(size_t i=0;i<authored;++i) {
            bool found=false;
            for(unsigned j=0;j<count;++j) {
                const auto* at=blob+44+j*12;short bone;unsigned offset;
                std::memcpy(&bone,at,2);if(bone!=parts[i].bone)continue;
                std::memcpy(&offset,at+8,4);D3DXMATRIX a,b;
                std::memcpy(&a,at+offset+lo*64,64);std::memcpy(&b,at+offset+hi*64,64);
                if(!GunAimMath::BlendMatrix(a,b,frame-lo,desired[i]))return false;
                if(parts[i].bone==0)desired[i]._42=native[i]._42+(desired[i]._42-bindHeight)*(native[i]._42/bindHeight);
                found=true;break;
            }
            if(!found)return false;
        }
        std::vector<D3DXMATRIX> fitted;
        if(!StealthAdditivePose::FitToNative(nodes,native,desired,authored,fitted))return false;
        if(blendFrom.size()==fitted.size() && frame<blendDuration) {
            // Interpolate in parent space so transitions cannot shorten arms
            // or stretch legs. World-matrix blending would distort the rig.
            std::vector<D3DXMATRIX> mixed(fitted.size());std::vector<bool> done(fitted.size(),false);
            size_t remaining=fitted.size();
            for(size_t pass=0;pass<fitted.size() && remaining;++pass) {
                bool progress=false;
                for(size_t i=0;i<fitted.size();++i) {
                    if(done[i])continue;const int parentIndex=nodes[i].parent;
                    if(parentIndex>=0 && !done[parentIndex])continue;
                    D3DXMATRIX from=blendFrom[i],to=fitted[i],inverse,local;
                    if(parentIndex>=0) {
                        if(!D3DXMatrixInverse(&inverse,nullptr,&blendFrom[parentIndex]))return false;
                        D3DXMatrixMultiply(&from,&blendFrom[i],&inverse);
                        if(!D3DXMatrixInverse(&inverse,nullptr,&fitted[parentIndex]))return false;
                        D3DXMatrixMultiply(&to,&fitted[i],&inverse);
                    }
                    const float t=frame/blendDuration;
                    if(!GunAimMath::BlendMatrix(from,to,t*t*(3-2*t),local))return false;
                    if(parentIndex>=0)D3DXMatrixMultiply(&mixed[i],&local,&mixed[parentIndex]);else mixed[i]=local;
                    done[i]=true;--remaining;progress=true;
                }
                if(!progress)return false;
            }
            if(remaining)return false;fitted.swap(mixed);
        } else blendFrom.clear();
        last=fitted;
        const auto basis=Basis(p);
        for(size_t i=0;i<parts.size();++i) {
            auto* part=parts[i].pointer;
            if(saved.empty() || saved.size()<parts.size())saved.push_back({parts[i],part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            D3DXMATRIX world;D3DXMatrixMultiply(&world,&fitted[i],&basis);
            std::memcpy(&part->m_LocalMatrix,&world,64);part->m_PartsFlag.m_Flag|=4;
        }
        return true;
    }
};
