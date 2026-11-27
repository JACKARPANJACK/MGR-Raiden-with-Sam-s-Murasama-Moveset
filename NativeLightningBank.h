#pragma once
#include <Hw.h>
#include "AnimationArchive.h"
#include <vector>
#include <string>
#include <cstdio>

// A dedicated native EFF/EFT bank. It is registered explicitly instead of
// depending on the engine's already-resident coreeff bank being replaced.
class NativeLightningBank
{
    std::vector<unsigned char> dat,dtt;
    Hw::cFmerge archive;
    bool registered=false;
    DWORD lastAttempt=0;
    static bool Read(const char* name,std::vector<unsigned char>& out)
    {
        char path[MAX_PATH]{};GetModuleFileNameA(nullptr,path,MAX_PATH);
        char* slash=std::strrchr(path,'\\');if(!slash)return false;slash[1]=0;
        FILE* file=nullptr;
        if(fopen_s(&file,(std::string(path)+"mods\\RaidenMoveset\\pl\\"+name).c_str(),"rb") || !file)return false;
        std::fseek(file,0,SEEK_END);const long size=std::ftell(file);std::rewind(file);
        if(size<32 || size>8*1024*1024){std::fclose(file);return false;}
        out.resize(size);const bool ok=std::fread(out.data(),1,out.size(),file)==out.size();std::fclose(file);
        AnimationArchive::Bank check;return ok && check.Parse(out);
    }
public:
    constexpr static unsigned Id=0x11C30;
    static NativeLightningBank& Get(){static NativeLightningBank bank;return bank;}
    bool Ready()
    {
        if(registered)return true;
        const DWORD now=GetTickCount();if(lastAttempt && now-lastAttempt<500)return false;lastAttempt=now;
        if(dat.empty() && (!Read("pl1c30.dat",dat) || !Read("pl1c30.dtt",dtt))) {dat.clear();dtt.clear();return false;}
        if(dat.empty() || dtt.empty())return false;
        archive.setData(reinterpret_cast<char*>(dat.data()),reinterpret_cast<char*>(dtt.data()));
        registered=reinterpret_cast<BOOL(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00C50)(Id,&archive)!=FALSE;
        return registered;
    }
    void SceneReleased()
    {
        if(registered)reinterpret_cast<void(__cdecl*)(unsigned,Hw::cFmerge*)>(shared::base+0xA00D60)(Id,&archive);
        registered=false;
        lastAttempt=0;
        // Keep the bytes resident: native controllers can finish shutdown after
        // resource-release notification. The next scene can register them again.
    }
};
