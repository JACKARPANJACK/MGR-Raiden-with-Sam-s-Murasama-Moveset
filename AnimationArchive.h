#pragma once
#include <windows.h>
#include "AnimationArchiveNames.generated.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <utility>

// Packed animation banks are read once. Callers retain their own member bytes
// for native playback; no loose-file extraction or script-directory links.
namespace AnimationArchive {
class Bank {
    std::map<std::string,std::vector<unsigned char>> members;
    bool ready=false;
    static unsigned Word(const unsigned char* p) {unsigned v;std::memcpy(&v,p,4);return v;}
public:
    bool Parse(const std::vector<unsigned char>& data) {
        members.clear();ready=false;
        if(data.size()<32 || std::memcmp(data.data(),"DAT\0",4))return false;
        const auto* b=data.data();
        const unsigned n=Word(b+4),offsets=Word(b+8),names=Word(b+16),sizes=Word(b+20);
        const auto fits=[&](size_t at,size_t size){return at<=data.size() && size<=data.size()-at;};
        if(!n || n>4096 || !fits(offsets,size_t(n)*4) || !fits(sizes,size_t(n)*4) || !fits(names,4))return false;
        const unsigned width=Word(b+names);
        if(!width || width>256 || !fits(size_t(names)+4,size_t(n)*width))return false;
        std::map<std::string,std::vector<unsigned char>> parsed;
        for(unsigned i=0;i<n;++i) {
            const char* name=reinterpret_cast<const char*>(b+names+4+i*width);
            if(!std::memchr(name,0,width) || !*name)return false;
            const unsigned at=Word(b+offsets+i*4),size=Word(b+sizes+i*4);
            if(!fits(at,size) || !parsed.emplace(name,std::vector<unsigned char>(b+at,b+at+size)).second)return false;
        }
        members=std::move(parsed);ready=true;return true;
    }
    bool Load(const char* relative) {
        if(ready)return true;
        char exe[MAX_PATH]{};if(!GetModuleFileNameA(nullptr,exe,MAX_PATH))return false;
        char* slash=std::strrchr(exe,'\\');if(!slash)return false;slash[1]=0;
        FILE* f=nullptr;if(fopen_s(&f,(std::string(exe)+relative).c_str(),"rb") || !f)return false;
        std::fseek(f,0,SEEK_END);const long size=std::ftell(f);std::rewind(f);
        if(size<32 || size>64*1024*1024){std::fclose(f);return false;}
        std::vector<unsigned char> data(size);
        const bool ok=std::fread(data.data(),1,data.size(),f)==data.size();std::fclose(f);
        return ok && Parse(data);
    }
    std::vector<unsigned char> Member(const std::string& name) const {
        const auto it=members.find(name);return it==members.end()?std::vector<unsigned char>{}:it->second;
    }
};
inline std::vector<unsigned char> Read(const char* bundle,const std::string& name) {
    static std::map<std::string,Bank> banks;
    const std::string path=std::string("mods\\RaidenMoveset\\pl\\")+bundle+".dat";
    auto& bank=banks[path];return bank.Load(path.c_str())?bank.Member(name):std::vector<unsigned char>{};
}
}
