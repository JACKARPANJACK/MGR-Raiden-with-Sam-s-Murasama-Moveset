#pragma once
#include <atomic>
#include <Windows.h>
#include <cstdio>
#include <cstring>
namespace SaiPullDiagnostics {
    inline std::atomic<const char*> status{"Ready: equip Sai and tap F / L3"};
    inline void Report(const char* message, unsigned state=0, unsigned enemy=0, float distance=0) {
        status=message;
        char path[MAX_PATH]{};
        if(!GetModuleFileNameA(nullptr,path,MAX_PATH)) return;
        char* slash=std::strrchr(path,'\\');if(!slash)return;
        strcpy_s(slash,size_t(path+MAX_PATH-slash),"\\scripts\\SaiPullRuntime.log");
        FILE* file=nullptr;
        if(!fopen_s(&file,path,"a") && file) {
            fprintf(file,"[%lu] %s state=%X target=%X distance=%.2f\n",GetTickCount(),message,state,enemy,distance);
            fclose(file);
        }
    }
}
