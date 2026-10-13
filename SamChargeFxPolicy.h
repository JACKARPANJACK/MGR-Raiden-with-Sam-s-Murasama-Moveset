#pragma once
#include <cstring>
namespace SamChargeFxPolicy
{
    inline bool Windup(const char* code)
    { return code && (!std::strcmp(code,"3000") || !std::strcmp(code,"3010") ||
        !std::strcmp(code,"3020") || !std::strcmp(code,"92e0")); }
    struct State
    {
        bool active=false,started=false;
        void Begin(){active=true;started=false;}
        void Reset(){*this={};}
        bool Start(bool ready){if(!active || started || !ready)return false;started=true;return true;}
    };
}
