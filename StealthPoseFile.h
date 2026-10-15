#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace StealthPoseFile {
struct Track {short bone;unsigned offset;};
struct Clip {
    std::vector<unsigned char> bytes;
    std::vector<Track> tracks;
    unsigned frames=0;
    float stride=0;
    template<class T> static T Read(const unsigned char* p) {T v;std::memcpy(&v,p,sizeof(v));return v;}
    bool Load(std::vector<unsigned char> data) {
        bytes.clear();tracks.clear();frames=0;stride=0;
        if(data.size()<44 || data.size()>16*1024*1024 || std::memcmp(data.data(),"STW1",4))return false;
        const auto* p=data.data();
        const unsigned n=Read<unsigned>(p+16),table=Read<unsigned>(p+12);
        const auto frameCount=Read<unsigned short>(p+10);
        const float travel=Read<float>(p+20);
        if(table!=44 || !n || n>4096 || frameCount<2 || frameCount>2048 ||
            !std::isfinite(travel) || travel<0 || travel>20 || table+size_t(n)*12>data.size())return false;
        std::vector<Track> parsed;
        for(unsigned i=0;i<n;++i) {
            const size_t at=table+i*12;
            const short bone=Read<short>(p+at);
            const unsigned count=Read<unsigned>(p+at+4),relative=Read<unsigned>(p+at+8);
            const size_t offset=at+size_t(relative);
            if(bone<0 || bone>0xfff || count!=frameCount || offset<table+size_t(n)*12 ||
                offset>data.size() || size_t(count)*64>data.size()-offset)return false;
            for(const auto& t:parsed)if(t.bone==bone)return false;
            for(unsigned frame=0;frame<count;++frame) {
                const auto* m=p+offset+frame*64;
                for(unsigned k=0;k<16;++k)if(!std::isfinite(Read<float>(m+k*4)))return false;
                if(std::abs(Read<float>(m+60)-1)>1e-4f ||
                    std::abs(Read<float>(m+12))+std::abs(Read<float>(m+28))+std::abs(Read<float>(m+44))>1e-4f)return false;
            }
            parsed.push_back({bone,unsigned(offset)});
        }
        bytes=std::move(data);tracks=std::move(parsed);frames=frameCount;stride=travel;
        return true;
    }
};
}
