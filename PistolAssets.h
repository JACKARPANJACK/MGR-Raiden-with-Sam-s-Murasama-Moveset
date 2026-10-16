#pragma once
#include "PistolPolicy.h"
#include "SamConfig.h"
#include "StealthPoseFile.h"
#include <array>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>

// The model bank is supplied through the existing custom-weapon factory hook.
// Keep bytes alive for the process lifetime, including while entities render.
namespace PistolAssets {
struct Bank {
    std::vector<char> data,textures;
    Hw::cFmerge archive;
    bool ready=false,paired=false;
};
inline std::array<Bank,PistolPolicy::Count> banks;
inline Bank animations;
inline unsigned Word(const char* p) {unsigned v;std::memcpy(&v,p,4);return v;}
inline bool ValidDat(const std::vector<char>& b) {
    if(b.size()<32 || std::memcmp(b.data(),"DAT\0",4))return false;
    const auto n=Word(b.data()+4),offsets=Word(b.data()+8),names=Word(b.data()+16),sizes=Word(b.data()+20);
    const auto fits=[&](size_t at,size_t count){return at<=b.size() && count<=b.size()-at;};
    if(!n || n>4096 || !fits(offsets,size_t(n)*4) || !fits(sizes,size_t(n)*4) || !fits(names,4))return false;
    if(!fits(Word(b.data()+12),size_t(n)*4))return false;
    const auto hash=Word(b.data()+24);
    if(!fits(hash,16))return false;
    const auto shift=Word(b.data()+hash);
    if(shift<19 || shift>31)return false;
    const size_t buckets=size_t(1)<<(31-shift);
    if(!fits(size_t(hash)+Word(b.data()+hash+4),buckets*2) ||
       !fits(size_t(hash)+Word(b.data()+hash+8),size_t(n)*4) ||
       !fits(size_t(hash)+Word(b.data()+hash+12),size_t(n)*2))return false;
    for(size_t i=0;i<buckets+n;++i) {
        const size_t at=i<buckets?size_t(hash)+Word(b.data()+hash+4)+i*2:
            size_t(hash)+Word(b.data()+hash+12)+(i-buckets)*2;
        short index;std::memcpy(&index,b.data()+at,2);
        if(index<-1 || index>=int(n) || (i>=buckets && index<0))return false;
    }
    const unsigned width=Word(b.data()+names);
    if(!width || width>256 || !fits(size_t(names)+4,size_t(n)*width))return false;
    for(unsigned i=0;i<n;++i) {
        if(!fits(Word(b.data()+offsets+i*4),Word(b.data()+sizes+i*4)))return false;
        if(!std::memchr(b.data()+names+4+i*width,0,width))return false;
    }
    return true;
}
inline bool LoadFile(Bank& bank,const char* relative,const char* required) {
    if(bank.ready)return true;
    std::string root=SamConfig::GetIniPath();root.resize(root.find_last_of("\\/")+1);
    FILE* f=nullptr;if(fopen_s(&f,(root+relative).c_str(),"rb") || !f)return false;
    fseek(f,0,SEEK_END);const long size=ftell(f);rewind(f);
    if(size<32 || size>32*1024*1024) {fclose(f);return false;}
    bank.data.resize(size);const bool ok=fread(bank.data.data(),1,size,f)==size_t(size);fclose(f);
    if(!ok || !ValidDat(bank.data))return false;
    bank.archive.setData(bank.data.data());
    bank.ready=bank.archive.getFileNameData(required)!=nullptr;
    return bank.ready;
}
inline std::vector<unsigned char> Member(const Bank& bank,const char* name) {
    if(!bank.ready)return {};
    const auto* b=bank.data.data();const unsigned n=Word(b+4),names=Word(b+16),width=Word(b+names);
    for(unsigned i=0;i<n;++i)if(!std::strcmp(b+names+4+i*width,name)) {
        const unsigned at=Word(b+Word(b+8)+i*4),size=Word(b+Word(b+20)+i*4);
        return std::vector<unsigned char>(b+at,b+at+size);
    }
    return {};
}
inline bool LoadAnimations() {
    static bool validated=false;if(validated)return true;
    if(!LoadFile(animations,"GameData\\wp\\pistol_anims.dat","pistol_ready.gwm"))return false;
    for(const char* name:{"pistol_ready.gwm","pistol_crouch.gwm","pistol_prone.gwm"}) {
        auto bytes=Member(animations,name);if(bytes.size()<44)return false;
        std::memcpy(bytes.data(),"STW1",4);StealthPoseFile::Clip clip;
        if(!clip.Load(std::move(bytes)))return false;
    }
    const auto grip=Member(animations,"pistol_grip.bin");if(grip.size()!=64)return false;
    for(unsigned i=0;i<16;++i) {float v;std::memcpy(&v,grip.data()+i*4,4);if(!std::isfinite(v))return false;}
    validated=true;return true;
}
inline void* Motion(const char* name) {return LoadAnimations()?animations.archive.getFileNameData(name):nullptr;}
inline bool Load(unsigned kind) {
    if(kind>=PistolPolicy::Count)return false;
    auto& bank=banks[kind];
    if(bank.paired)return true;
    char relative[64],model[32];
    std::snprintf(relative,sizeof(relative),"GameData\\wp\\wp%04x.dat",PistolPolicy::Get(kind).object&0xffff);
    std::snprintf(model,sizeof(model),"wp%04x.wmb",PistolPolicy::Get(kind).object&0xffff);
    if(!LoadFile(bank,relative,model))return false;
    std::string root=SamConfig::GetIniPath();root.resize(root.find_last_of("\\/")+1);
    std::snprintf(relative,sizeof(relative),"GameData\\wp\\wp%04x.dtt",PistolPolicy::Get(kind).object&0xffff);
    FILE* f=nullptr;if(fopen_s(&f,(root+relative).c_str(),"rb") || !f)return false;
    fseek(f,0,SEEK_END);const long size=ftell(f);rewind(f);
    if(size<32 || size>32*1024*1024) {fclose(f);return false;}
    bank.textures.resize(size);
    const bool ok=fread(bank.textures.data(),1,size,f)==size_t(size);fclose(f);
    if(!ok || !ValidDat(bank.textures))return false;
    // Fmerge is a DAT/DTT pair. A WTA in DAT alone cannot resolve its WTP.
    // Keep both buffers alive while the native model and textures reference them.
    bank.archive.setData(bank.data.data(),bank.textures.data());
    char wta[32],wtp[32];
    std::snprintf(wta,sizeof(wta),"wp%04x.wta",PistolPolicy::Get(kind).object&0xffff);
    std::snprintf(wtp,sizeof(wtp),"wp%04x.wtp",PistolPolicy::Get(kind).object&0xffff);
    bank.paired=bank.archive.getFileNameData(wta) && bank.archive.getFileNameData(wtp);
    return bank.paired;
}
inline bool Supply(unsigned id,Hw::cFmerge& out) {
    if(!PistolPolicy::Object(id))return false;
    const unsigned kind=id-PistolPolicy::Get(0).object;
    if(!Load(kind))return false;
    out=banks[kind].archive;return true;
}
}
