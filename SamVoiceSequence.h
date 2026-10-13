#pragma once
#include "SamBossSequence.h"
#include "SamVoicePolicy.h"
namespace SamVoiceSequence
{
    inline std::vector<uint8_t> Adapt(const void* data,size_t available)
    {
        using namespace SamBossSequence;
        if(!data || available<16)return {};
        const auto* src=static_cast<const uint8_t*>(data);
        const size_t size=Size(src);if(!size || size>available)return {};
        const unsigned nodes=U16(src+8),pairs=U16(src+10),ps=16+nodes*8,ts=ps+pairs*4,strings=U32(src+12);
        auto str=[&](unsigned offset)->const char* {
            return offset<strings && std::memchr(src+ts+offset,0,strings-offset) ? reinterpret_cast<const char*>(src+ts+offset) : "";
        };
        std::vector<uint8_t> out(src,src+size);
        for(unsigned i=0;i<nodes;++i) {
            const auto* node=src+16+i*8;
            const unsigned index=U16(node+6),attrs=U16(node+4),first=U16(node+2),count=U16(node);
            if(index+attrs>=pairs || first+count>nodes)return {};
            if(std::strcmp(str(U16(src+ps+index*4)),"SeTrack"))continue;
            for(unsigned c=first;c<first+count;++c) {
                const auto* child=src+16+c*8;const unsigned ci=U16(child+6),ca=U16(child+4);
                if(ci+ca>=pairs)return {};
                for(unsigned a=1;a<=ca;++a) {
                    const auto* entry=src+ps+(ci+a)*4;
                    if(std::strcmp(str(U16(entry)),"EventName"))continue;
                    const char* original=str(U16(entry+2));const char* mapped=SamVoicePolicy::Attack(original);
                    if(!std::strcmp(original,mapped))continue;
                    const unsigned offset=unsigned(out.size())-ts;
                    if(offset+std::strlen(mapped)+1>=65535)return {};
                    out.insert(out.end(),mapped,mapped+std::strlen(mapped)+1);
                    W16(out.data()+ps+(ci+a)*4+2,offset);
                }
            }
        }
        W32(out.data()+12,unsigned(out.size())-ts);
        return out;
    }
}
