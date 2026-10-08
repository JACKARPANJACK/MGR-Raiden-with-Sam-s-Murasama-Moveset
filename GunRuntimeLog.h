#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace GunRuntimeLog
{
    inline bool selected=false;
    inline const char* stage="startup";
    inline void Write(const char* format,...)
    {
        char message[768];va_list args;va_start(args,format);
        vsnprintf_s(message,sizeof(message),_TRUNCATE,format,args);va_end(args);
        char path[MAX_PATH];if(!GetModuleFileNameA(nullptr,path,MAX_PATH)) return;
        char* slash=std::strrchr(path,'\\');if(!slash) return;*slash=0;
        char file[MAX_PATH];sprintf_s(file,"%s\\scripts\\GunRuntime.log",path);
        FILE* stream=nullptr;fopen_s(&stream,file,"a");if(!stream) return;
        fprintf(stream,"[%lu] %s\n",GetTickCount(),message);fclose(stream);
    }
    inline int Exception(EXCEPTION_POINTERS* info,const char* phase)
    {
        // The frame hook must remain fail-safe, but gun failures must be visible.
        if(selected && info && info->ExceptionRecord)
        {
            static DWORD last=0;
            const DWORD now=GetTickCount();
            if(!last || now-last>=2000)
            {
                last=now;
                Write("EXCEPTION phase=%s stage=%s code=%08lX address=%p",phase,stage,
                    info->ExceptionRecord->ExceptionCode,info->ExceptionRecord->ExceptionAddress);
            }
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }
}
