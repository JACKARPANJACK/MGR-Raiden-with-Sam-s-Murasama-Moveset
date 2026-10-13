#pragma once
#include <cstring>

namespace SamLightComboPolicy
{
    constexpr unsigned MoveIndex = 19;
    inline bool ExtensionCode(const char* code)
    {return code && std::strlen(code)==4 && !std::strncmp(code,"900",3) && code[3]>='0' && code[3]<='3';}
    inline bool NativeEnder(const char* name)
    {
        if (!name) return false;
        if (!std::strncmp(name,"pl1400_",7)) name += 7;
        if(std::strncmp(name,"2003",4))return false;
        const char* suffix=name+4;
        return !*suffix || !std::strcmp(suffix,".mot") || !std::strcmp(suffix,"_2_seq.bxm");
    }
    struct Buffer
    {
        bool pending = false;
        void Reset() { pending = false; }
        bool Poll(bool eligible, bool neutralLight, bool conflictingAttack)
        {
            if (!eligible || conflictingAttack) Reset();
            else if (neutralLight) pending = true;
            return pending;
        }
    };
}
