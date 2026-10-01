#pragma once
#include "SamSheathPolicy.h"
#include "SamTogglePolicy.h"
namespace SamDlcRouting
{
    inline bool SharedMotion(const char* code)
    {
        if(!code) return false;
        static constexpr const char* sharedCodes[]={"8550","8552","8553","8585","8960","8970","8980"};
        for(const char* shared:sharedCodes)
            if(!std::strcmp(code,shared)) return true;
        return false;
    }
    inline bool Code(char* result,const char* name,bool active,uint32_t state,bool bladeMode)
    {
        if(!result) return false;
        result[0]=0;
        return active && (SamTogglePolicy::OwnsAction(state)||bladeMode) && SamSheathPolicy::Code(result,name);
    }
}
