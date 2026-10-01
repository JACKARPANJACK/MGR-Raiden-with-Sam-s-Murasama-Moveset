#pragma once
#include <shared.h>
#include "injector/injector.hpp"
#include <cstring>
#include "WeaponBehaviorLoad.h"

// Native slow-rate propagation assumes both optional weapon Behaviors exist.
// Crash dumps 28744/1264/26300 all fault at 7871BF with EAX=0. Preserve native
// timing and rate propagation; make only the eight optional-weapon loads null-safe.
namespace WeaponSlowMotionFix
{
    inline constexpr unsigned Sites[]={0x7871BF,0x787208,0x78731D,0x78736E,0x787491,0x7874E2,0x7875FF,0x787650};
    inline bool Install()
    {
        // Validate every instruction before changing any site. Each original
        // six-byte MOV is followed by TEST of the destination and a native skip.
        for(unsigned rva:Sites)
        {
            const unsigned char expected[]={0x8B,static_cast<unsigned char>(rva==0x787208?0xB0:0xB8),0xF0,4,0,0,
                0x85,static_cast<unsigned char>(rva==0x787208?0xF6:0xFF)};
            if(std::memcmp(reinterpret_cast<void*>(shared::base+rva),expected,sizeof(expected))) return false;
        }
        for(unsigned rva:Sites)
        {
            injector::MakeCALL(shared::base+rva,rva==0x787208?WeaponBehaviorLoad::LoadESI:WeaponBehaviorLoad::LoadEDI);
            injector::WriteMemory<unsigned char>(shared::base+rva+5,0x90,true);
        }
        return true;
    }
}
