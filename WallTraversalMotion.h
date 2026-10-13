#pragma once
#include "WallClingPose.h"
#include <vector>
#include <array>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace WallTraversalMotion {
    enum class Variant {Cling,RunUp,RunSideLeft,RunSideRight,SlideDown,SlideSideLeft,SlideSideRight,Launch};
    inline uint16_t U16(const unsigned char* p) {uint16_t v;std::memcpy(&v,p,2);return v;}
    inline unsigned U32(const unsigned char* p) {unsigned v;std::memcpy(&v,p,4);return v;}
    inline float F32(const unsigned char* p) {float v;std::memcpy(&v,p,4);return v;}
    inline float PgHalf(uint16_t word) {
        if((word&0x7FFFu)==0)return 0.0f;
        unsigned exp=(word>>9)&63u;
        if(exp==63u)return 0.0f;
        uint32_t bits=(uint32_t(word&0x8000u)<<16)|(uint32_t(exp+80u)<<23)|(uint32_t(word&511u)<<14);
        float out;std::memcpy(&out,&bits,4);return out;
    }
    inline float SampleChannel(const unsigned char* data,size_t size,unsigned recPos,unsigned frame,float fallback) {
        const auto* r=data+recPos;
        unsigned char fmt=r[3];
        unsigned keyCount=U16(r+4);
        if(fmt==0)return F32(r+8);
        unsigned curveRel=U32(r+8);
        if(!keyCount||curveRel>size-recPos)return fallback;
        unsigned cp=recPos+curveRel;
        if(fmt==1) {
            if(cp+keyCount*4u>size)return fallback;
            unsigned idx=(std::min)(frame,keyCount-1u);
            return F32(data+cp+idx*4u);
        }
        if(fmt==2) {
            if(cp+8u+keyCount*2u>size)return fallback;
            unsigned idx=(std::min)(frame,keyCount-1u);
            return F32(data+cp)+F32(data+cp+4u)*float(U16(data+cp+8u+idx*2u));
        }
        if(fmt==3) {
            if(cp+4u+keyCount>size)return fallback;
            unsigned idx=(std::min)(frame,keyCount-1u);
            return PgHalf(U16(data+cp))+PgHalf(U16(data+cp+2u))*float(data[cp+4u+idx]);
        }
        struct Knot {unsigned f;float p,m0,m1;};
        std::vector<Knot> keys;keys.reserve(keyCount);
        if(fmt==4) {
            if(cp+keyCount*16u>size)return fallback;
            for(unsigned i=0;i<keyCount;++i) {
                const auto* k=data+cp+i*16u;
                keys.push_back({U16(k),F32(k+4),F32(k+8),F32(k+12)});
            }
        } else if(fmt>=5&&fmt<=8) {
            float p0=0,dp=0,m0=0,dm0=0,m1=0,dm1=0;
            unsigned kpos=cp;
            if(fmt==5) {
                if(cp+24u+keyCount*8u>size)return fallback;
                p0=F32(data+cp);dp=F32(data+cp+4);m0=F32(data+cp+8);dm0=F32(data+cp+12);m1=F32(data+cp+16);dm1=F32(data+cp+20);
                kpos=cp+24u;
            } else {
                unsigned stride=fmt==8?5u:4u;
                if(cp+12u+keyCount*stride>size)return fallback;
                p0=PgHalf(U16(data+cp));dp=PgHalf(U16(data+cp+2));
                m0=PgHalf(U16(data+cp+4));dm0=PgHalf(U16(data+cp+6));
                m1=PgHalf(U16(data+cp+8));dm1=PgHalf(U16(data+cp+10));
                kpos=cp+12u;
            }
            unsigned last=0;
            for(unsigned i=0;i<keyCount;++i) {
                unsigned kf=0,qv=0,qa=0,qb=0;
                if(fmt==5) {
                    const auto* k=data+kpos+i*8u;
                    kf=U16(k);qv=U16(k+2);qa=U16(k+4);qb=U16(k+6);
                } else if(fmt==8) {
                    const auto* k=data+kpos+i*5u;
                    kf=( unsigned(k[0])<<8 )|unsigned(k[1]);qv=k[2];qa=k[3];qb=k[4];
                } else {
                    const auto* k=data+kpos+i*4u;
                    kf=k[0];qv=k[1];qa=k[2];qb=k[3];
                    if(fmt==7){kf+=last;last=kf;}
                }
                keys.push_back({kf,p0+dp*float(qv),m0+dm0*float(qa),m1+dm1*float(qb)});
            }
        } else return fallback;
        if(frame<=keys.front().f)return keys.front().p;
        for(size_t i=0;i+1<keys.size();++i) {
            if(frame<=keys[i+1].f) {
                float dt=float(keys[i+1].f-keys[i].f);
                if(dt<=0.0f)return keys[i].p;
                float t=(float(frame)-float(keys[i].f))/dt,t2=t*t,t3=t2*t;
                return (2*t3-3*t2+1)*keys[i].p+(t3-2*t2+t)*dt*keys[i].m1+(-2*t3+3*t2)*keys[i+1].p+(t3-t2)*dt*keys[i+1].m0;
            }
        }
        return keys.back().p;
    }
    inline void Constant(unsigned char* record,float value) {
        record[3]=0;std::memset(record+4,0,4);std::memcpy(record+8,&value,4);
    }
    struct LegTransform {float r[3][3];float t[3];};
    inline LegTransform TransformLeg(const float* v) {
        float sx=std::sin(v[3]),cx=std::cos(v[3]),sy=std::sin(v[4]),cy=std::cos(v[4]),sz=std::sin(v[5]),cz=std::cos(v[5]);
        return {{{cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx},
                 {sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx},
                 {-sy,cy*sx,cy*cx}},{v[0],v[1],v[2]}};
    }
    inline LegTransform ComposeLeg(const LegTransform& a,const LegTransform& b) {
        LegTransform out{};
        for(int i=0;i<3;++i) {
            out.t[i]=a.t[i];
            for(int k=0;k<3;++k)out.t[i]+=a.r[i][k]*b.t[k];
            for(int j=0;j<3;++j)for(int k=0;k<3;++k)out.r[i][j]+=a.r[i][k]*b.r[k][j];
        }
        return out;
    }
    inline bool LockFeet(std::vector<unsigned char>& motion,Variant variant,float wallDistance) {
        if(variant==Variant::Launch)return true;
        const unsigned table=U32(motion.data()+12),count=U32(motion.data()+16),frames=U16(motion.data()+10);
        int records[23][6];for(auto& bone:records)for(int& rec:bone)rec=-1;
        for(unsigned i=0;i<count;++i) {
            const auto* rec=motion.data()+table+i*12u;short bone;std::memcpy(&bone,rec,2);
            if(bone>=0&&bone<23&&rec[2]<6)records[bone][rec[2]]=int(table+i*12u);
        }
        const bool left=variant==Variant::RunSideLeft||variant==Variant::SlideSideLeft;
        const bool right=variant==Variant::RunSideRight||variant==Variant::SlideSideRight;
        const unsigned axis=(left||right)?0u:2u;const float sign=right?-1.0f:1.0f;
        if(records[0][axis]<0)return false;
        std::vector<float> corrected(frames);
        auto sample=[&](int bone,int prop,unsigned frame,float fallback) {
            return records[bone][prop]>=0?SampleChannel(motion.data(),motion.size(),unsigned(records[bone][prop]),frame,fallback):fallback;
        };
        // Raiden's native hip/knee/ankle/toe chains, from the original WMB.
        // Sole landmarks are below the ankle/toe joints, not their centers.
        const float bind[4][3]={{0,0,0},{0,-.454649985f,-.022f},{0,-.460949987f,-.026f},{0,-.114450008f,.108f}};
        for(unsigned frame=0;frame<frames;++frame) {
            float root[6];const float rootDefault[6]={0,1.065361977f,0,0,0,0};
            for(int p=0;p<6;++p)root[p]=sample(0,p,frame,rootDefault[p]);
            LegTransform pelvis=TransformLeg(root);
            float reach=-100.0f;
            for(int side=0;side<2;++side) {
                LegTransform world=pelvis;
                for(int joint=0;joint<4;++joint) {
                    int bone=15+side*4+joint;float v[6];
                    for(int p=0;p<6;++p) {
                        float fallback=p<3?bind[joint][p]:0.0f;
                        if(joint==0&&p==0)fallback=side?.096f:-.096f;
                        v[p]=sample(bone,p,frame,fallback);
                    }
                    world=ComposeLeg(world,TransformLeg(v));
                    if(joint>=2) {
                        const float soleY=joint==2?-.11f:-.035f,soleZ=joint==2?-.055f:0.0f;
                        float contact=world.t[axis]+world.r[axis][1]*soleY+world.r[axis][2]*soleZ;
                        reach=(std::max)(reach,sign*contact);
                    }
                }
            }
            const float shift=sign*(wallDistance-reach);
            if(!std::isfinite(shift)||std::fabs(shift)>.6f)return false;
            corrected[frame]=root[axis]+shift;
        }
        const unsigned recOffset=unsigned(records[0][axis]);
        auto bounds=std::minmax_element(corrected.begin(),corrected.end());
        if(*bounds.second-*bounds.first<1e-6f)Constant(motion.data()+recOffset,corrected[0]);
        else {
            unsigned relative=unsigned(motion.size())-recOffset;uint16_t keyCount=uint16_t(frames),zero=0;
            auto* rec=motion.data()+recOffset;rec[3]=1;std::memcpy(rec+4,&keyCount,2);std::memcpy(rec+6,&zero,2);std::memcpy(rec+8,&relative,4);
            const auto* data=reinterpret_cast<const unsigned char*>(corrected.data());motion.insert(motion.end(),data,data+frames*sizeof(float));
        }
        return true;
    }
    inline std::vector<unsigned char> CopyMotion(const void* source,size_t size,Variant variant,float wallDistance=.47f) {
        if(!source||size<44||size>4*1024*1024)return {};
        if(!std::isfinite(wallDistance)||wallDistance<.35f||wallDistance>.7f)return {};
        const auto* data=static_cast<const unsigned char*>(source);
        unsigned frames=U16(data+10),offset=U32(data+12),count=U32(data+16);
        if(std::memcmp(data,"mot\0",4)||!frames||frames>4096||count>4096||offset>size||count*12>size-offset)return {};
        const bool cling=variant==Variant::Cling;
        const bool rewriteBone0=!cling&&variant!=Variant::Launch;
        std::vector<WallClingPose::Channel> additions;
        int bone0Idx[6]={-1,-1,-1,-1,-1,-1};
        for(unsigned i=0;i<count;++i) {
            const auto* r=data+offset+i*12;short bone;std::memcpy(&bone,r,2);
            if(bone==0&&r[2]<6)bone0Idx[r[2]]=int(i);
        }
        if(cling) {
            for(const auto& channel:WallClingPose::Channels) {
                bool found=false;
                for(unsigned i=0;i<count;++i) {
                    const auto* r=data+offset+i*12;short bone;std::memcpy(&bone,r,2);
                    if(bone==channel.bone&&r[2]==channel.property){found=true;break;}
                }
                if(!found)additions.push_back(channel);
            }
        } else if(rewriteBone0) {
            for(unsigned char p=0;p<6;++p)
                if(bone0Idx[p]<0)additions.push_back({0,p,0.0f});
        }
        std::array<std::vector<float>,6> b0{};
        if(rewriteBone0) {
            for(int p=0;p<6;++p)b0[p].resize(frames,0.0f);
            float pitch=0.0f,dx=0.0f,dy=0.0f,dz=0.0f;
            switch(variant) {
            case Variant::RunUp:pitch=-1.35f;dx=0.0f;dy=.76f;dz=.45f;break;
            case Variant::RunSideLeft:pitch=0.0f;dx=-.14f;dy=.32f;dz=0.0f;break;
            case Variant::RunSideRight:pitch=0.0f;dx=.11f;dy=.35f;dz=0.0f;break;
            case Variant::SlideDown:
                if(frames==24){pitch=-1.35f;dx=0.0f;dy=.78f;dz=.45f;}
                else {pitch=0.0f;dx=0.0f;dy=-.10f;dz=.14f;}
                break;
            case Variant::SlideSideLeft:pitch=0.0f;dx=-.08f;dy=.32f;dz=0.0f;break;
            case Variant::SlideSideRight:pitch=0.0f;dx=.05f;dy=.35f;dz=0.0f;break;
            default:break;
            }
            const float defaults[6]={0.0f,.9963f,0.0f,0.0f,0.0f,0.0f};
            const float cp=std::cos(pitch),sp=std::sin(pitch);
            for(unsigned f=0;f<frames;++f) {
                float v[6];
                for(int p=0;p<6;++p)
                    v[p]=bone0Idx[p]>=0?SampleChannel(data,size,offset+unsigned(bone0Idx[p])*12u,f,defaults[p]):defaults[p];
                b0[0][f]=v[0]+dx;
                b0[1][f]=cp*v[1]-sp*v[2]+dy;
                b0[2][f]=sp*v[1]+cp*v[2]+dz;
                if(std::fabs(pitch)<1e-5f) {
                    b0[3][f]=v[3];b0[4][f]=v[4];b0[5][f]=v[5];
                } else {
                    const float sx=std::sin(v[3]),cx=std::cos(v[3]);
                    const float sy=std::sin(v[4]),cy=std::cos(v[4]);
                    const float sz=std::sin(v[5]),cz=std::cos(v[5]);
                    const float r00=cz*cy;
                    const float r10=sz*cy,r11=sz*sy*sx+cz*cx,r12=sz*sy*cx-cz*sx;
                    const float r20=-sy,  r21=cy*sx,         r22=cy*cx;
                    const float n10=cp*r10-sp*r20,n11=cp*r11-sp*r21,n12=cp*r12-sp*r22;
                    const float n20=sp*r10+cp*r20,n21=sp*r11+cp*r21,n22=sp*r12+cp*r22;
                    const float ny=std::asin((std::clamp)(-n20,-1.0f,1.0f));
                    if(std::cos(ny)>1e-4f) {
                        b0[3][f]=std::atan2(n21,n22);b0[4][f]=ny;b0[5][f]=std::atan2(n10,r00);
                    } else {
                        b0[3][f]=std::atan2(-n12,n11);b0[4][f]=ny;b0[5][f]=0.0f;
                    }
                }
            }
        }
        unsigned total=count+unsigned(additions.size()),table=44,storage=table+(total+1)*12;
        std::vector<unsigned char> result(storage+size,0);
        result.reserve(storage+size+6u*frames*4u);
        std::memcpy(result.data(),data,44);std::memcpy(result.data()+12,&table,4);std::memcpy(result.data()+16,&total,4);
        std::memcpy(result.data()+storage,data,size);
        auto writeBone0=[&](unsigned recIdx,unsigned char prop) {
            const auto& vals=b0[prop];
            auto minmax=std::minmax_element(vals.begin(),vals.end());
            unsigned recOffset=table+recIdx*12u;
            if(*minmax.second-*minmax.first<1e-6f) {
                Constant(result.data()+recOffset,vals[0]);
            } else {
                unsigned rel=unsigned(result.size())-recOffset;
                uint16_t f16=uint16_t(frames),z16=0;
                auto* dest=result.data()+recOffset;
                dest[3]=1;std::memcpy(dest+4,&f16,2);std::memcpy(dest+6,&z16,2);std::memcpy(dest+8,&rel,4);
                const auto* bytes=reinterpret_cast<const unsigned char*>(vals.data());
                result.insert(result.end(),bytes,bytes+frames*sizeof(float));
            }
        };
        for(unsigned i=0;i<count;++i) {
            const auto* r=data+offset+i*12;auto* dest=result.data()+table+i*12;
            std::memcpy(dest,r,12);short bone;std::memcpy(&bone,r,2);
            if(r[3]!=0) {
                unsigned curve=U32(r+8);
                if(curve>size-offset-i*12)return {};
                unsigned relocated=storage+offset+i*12+curve-(table+i*12);
                std::memcpy(dest+8,&relocated,4);
            }
            if(bone==-1&&r[2]<=5)Constant(dest,0);
            if(cling) {
                for(const auto& channel:WallClingPose::Channels)
                    if(bone==channel.bone&&r[2]==channel.property)Constant(dest,channel.value);
            } else if(rewriteBone0&&bone==0&&r[2]<6) {
                writeBone0(i,r[2]);
            }
        }
        for(unsigned i=0;i<additions.size();++i) {
            unsigned recIdx=count+i;
            auto* dest=result.data()+table+recIdx*12;
            std::memcpy(dest,&additions[i].bone,2);dest[2]=additions[i].property;
            if(cling)Constant(dest,additions[i].value);
            else writeBone0(recIdx,additions[i].property);
        }
        short end=32767;std::memcpy(result.data()+table+total*12,&end,2);
        if(!LockFeet(result,variant,wallDistance))return {};
        return result;
    }
    inline std::vector<unsigned char> CopyMotion(const void* source,size_t size,bool cling) {
        return CopyMotion(source,size,cling?Variant::Cling:Variant::RunUp);
    }
}
