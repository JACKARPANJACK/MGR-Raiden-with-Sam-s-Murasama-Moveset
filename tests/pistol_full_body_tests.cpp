#include <d3dx9math.h>
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>
#include <cstring>
namespace Hw {using cMtx=D3DXMATRIX;}
struct cParts {D3DXMATRIX m_LocalMatrix;struct {unsigned short m_Flag=0;} m_PartsFlag;cParts* m_pPartsParent=nullptr;short id=-1;};
struct Pl0000;
struct Entity {Pl0000* m_pBehavior=nullptr;};
struct EntityHandle {Entity* p=nullptr;EntityHandle& operator=(Entity* v){p=v;return *this;}Entity* getEntity()const{return p;}};
struct Pl0000 {
    Entity* m_pEntity=nullptr;Pl0000* m_pParent=nullptr;D3DXVECTOR3 m_Rot{0,0,0},m_TransPos{0,0,0};
    struct {int m_BoneNum=0;cParts** m_ppBones=nullptr;cParts* m_pBones=nullptr;} m_PartsData;
    cParts* getPartsPtr(short id){for(int j=0;j<m_PartsData.m_BoneNum;++j)if(m_PartsData.m_pBones[j].id==id)return m_PartsData.m_pBones+j;return nullptr;}
};
#include "../PistolFullBodyPose.h"
static D3DXMATRIX Sample(const std::vector<unsigned char>& data,short bone,unsigned frame) {
    unsigned n;std::memcpy(&n,data.data()+16,4);
    for(unsigned j=0;j<n;++j) {
        const auto* at=data.data()+44+j*12;short id;unsigned offset;
        std::memcpy(&id,at,2);std::memcpy(&offset,at+8,4);
        if(id==bone){D3DXMATRIX m;std::memcpy(&m,at+offset+frame*64,64);return m;}
    }
    assert(false);return {};
}
int main() {
    struct Bone {int id,parent;float x,y,z;};
    std::ifstream input("Release/PistolBodyFixtures/bones.txt");unsigned count;input>>count;assert(input && count>22);
    std::vector<Bone> bones(count);for(auto& b:bones)input>>b.id>>b.parent>>b.x>>b.y>>b.z;
    unsigned framesTested=0;
    std::ifstream catalog("Release/PistolBodyFixtures/clips.txt");assert(catalog);
    std::string code;unsigned clipsTested=0;
    while(catalog>>code) {
        ++clipsTested;
        std::ifstream file(std::string("Release/PistolBodyFixtures/")+code+".fbw",std::ios::binary);
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),{});assert(data.size()>44);
        unsigned short frameCount;std::memcpy(&frameCount,data.data()+10,2);
        float referenceHeight;std::memcpy(&referenceHeight,data.data()+36,4);
        for(float scale:{1.0f,1.12f,.91f}) {
            Pl0000 player;Entity entity{&player};player.m_pEntity=&entity;
            player.m_Rot.y=.73f;player.m_TransPos={23,-7,51};
            D3DXMATRIX actor;D3DXMatrixRotationY(&actor,player.m_Rot.y);actor._41=23;actor._42=-7;actor._43=51;
            std::vector<cParts> parts(count+1);std::vector<D3DXMATRIX> rest(count+1);
            for(unsigned j=0;j<count;++j) {
                parts[j].id=short(bones[j].id);D3DXMATRIX local;
                D3DXMatrixTranslation(&local,bones[j].x*scale,bones[j].y*scale,bones[j].z*scale);
                if(bones[j].parent>=0){parts[j].m_pPartsParent=&parts[bones[j].parent];rest[j]=local*rest[bones[j].parent];}else rest[j]=local;
                parts[j].m_LocalMatrix=rest[j]*actor;parts[j].m_PartsFlag.m_Flag=0x20;
            }
            parts[count].id=3000;parts[count].m_pPartsParent=&parts[3];
            D3DXMatrixTranslation(&rest[count],.04f,.02f,.01f);rest[count]*=rest[3];parts[count].m_LocalMatrix=rest[count]*actor;
            player.m_PartsData.m_BoneNum=int(parts.size());player.m_PartsData.m_pBones=parts.data();
            PistolFullBodyPose pose;assert(pose.Capture(&player,data.data()));
            pose.BeginStage(6);
            for(unsigned f=0;f<=6;++f) {
                assert(pose.Apply(&player,data.data(),float(f)));
                for(int id:{8,9,12,13,16,17,20,21}) {
                    auto* part=player.getPartsPtr(short(id));const auto& a=part->m_LocalMatrix;const auto& b=part->m_pPartsParent->m_LocalMatrix;
                    const D3DXVECTOR3 delta(a._41-b._41,a._42-b._42,a._43-b._43);const auto& bind=bones[unsigned(part-parts.data())];
                    assert(std::fabs(D3DXVec3Length(&delta)-std::sqrt(bind.x*bind.x+bind.y*bind.y+bind.z*bind.z)*scale)<.0002f);
                }
                if(f==0 || (f==6 && scale==1))for(int id:{0,3,9,13,17,21}) {
                    auto* part=player.getPartsPtr(short(id));const auto expected=f==0?rest[unsigned(part-parts.data())]*actor:Sample(data,short(id),f)*actor;
                    for(unsigned k=0;k<16;++k)assert(std::fabs(((float*)&expected)[k]-((float*)&part->m_LocalMatrix)[k])<.0003f);
                }
                pose.BeginFrame();++framesTested;
            }
            pose.End();assert(pose.Capture(&player,data.data()));
            for(unsigned f=0;f<frameCount;++f) {
                // Simulate another native graph leaving a completely wrong
                // pose; the final attack pass must replace both body halves.
                for(auto& p:parts)D3DXMatrixTranslation(&p.m_LocalMatrix,-20,-30,-40);
                const auto before=parts;
                if(f==4)assert(pose.Capture(&player,data.data())); // repeated routes retain the original costume calibration
                assert(pose.Apply(&player,data.data(),float(f)));
                for(int id:{0,1,3,5,7,8,9,11,12,13,15,16,17,19,20,21}) {
                    auto* part=player.getPartsPtr(short(id));assert(part && (part->m_PartsFlag.m_Flag&4));
                    if(scale==1) {
                        const auto expected=Sample(data,short(id),f)*actor;
                        for(unsigned k=0;k<16;++k)assert(std::fabs(((float*)&expected)[k]-((float*)&part->m_LocalMatrix)[k])<.0003f);
                    }
                }
                for(int id:{8,9,12,13,16,17,20,21}) {
                    auto* part=player.getPartsPtr(short(id));const auto& a=part->m_LocalMatrix;const auto& b=part->m_pPartsParent->m_LocalMatrix;
                    const D3DXVECTOR3 delta(a._41-b._41,a._42-b._42,a._43-b._43);
                    const float length=D3DXVec3Length(&delta);
                    const auto& bind=bones[unsigned(part-parts.data())];
                    assert(std::fabs(length-std::sqrt(bind.x*bind.x+bind.y*bind.y+bind.z*bind.z)*scale)<.0002f);
                }
                const auto pelvis=Sample(data,0,f);
                assert(std::fabs(player.getPartsPtr(0)->m_LocalMatrix._42-(player.m_TransPos.y+pelvis._42*scale))<.0002f);
                D3DXMATRIX extraLocal;D3DXMatrixTranslation(&extraLocal,.04f,.02f,.01f);
                const auto extraExpected=extraLocal*parts[3].m_LocalMatrix;
                for(unsigned k=0;k<16;++k)assert(std::fabs(((float*)&extraExpected)[k]-((float*)&parts[count].m_LocalMatrix)[k])<.0003f);
                pose.BeginFrame();
                for(unsigned j=0;j<parts.size();++j){assert(!std::memcmp(&parts[j].m_LocalMatrix,&before[j].m_LocalMatrix,64));assert(parts[j].m_PartsFlag.m_Flag==before[j].m_PartsFlag.m_Flag);}
                ++framesTested;
            }
            pose.BeginStage();
            for(unsigned f=0;f<=4;++f) {
                assert(pose.Apply(&player,data.data(),float(f)));
                for(int id:{8,9,12,13,16,17,20,21}) {
                    auto* part=player.getPartsPtr(short(id));const auto& a=part->m_LocalMatrix;const auto& b=part->m_pPartsParent->m_LocalMatrix;
                    const D3DXVECTOR3 delta(a._41-b._41,a._42-b._42,a._43-b._43);const auto& bind=bones[unsigned(part-parts.data())];
                    assert(std::fabs(D3DXVec3Length(&delta)-std::sqrt(bind.x*bind.x+bind.y*bind.y+bind.z*bind.z)*scale)<.0002f);
                }
                if(scale==1) {
                    const auto expected=Sample(data,13,f==0?frameCount-1:f)*actor;
                    if(f==0 || f==4)for(unsigned k=0;k<16;++k)assert(std::fabs(((float*)&expected)[k]-((float*)&player.getPartsPtr(13)->m_LocalMatrix)[k])<.0003f);
                }
                pose.BeginFrame();++framesTested;
            }
            pose.Reset();
        }
    }
    std::printf("PASS: %u full-body frames; all Humanoid attack clips, three costume sizes, actor transforms, limb lengths and exact restoration\n",framesTested);
    assert(clipsTested==16);
}
