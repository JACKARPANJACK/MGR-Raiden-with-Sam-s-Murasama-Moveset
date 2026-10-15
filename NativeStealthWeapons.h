#pragma once
#include "StealthStanceController.h"
namespace NativeStealthWeapons {
using Release=uintptr_t(__thiscall*)(Pl0000*);
using Throw=uintptr_t(__thiscall*)(Pl0000*,float,float);
inline Release launcher=nullptr;
inline Throw grenade=nullptr;
inline bool installed=false;
inline thread_local bool nested=false;
inline uintptr_t __fastcall Launcher(Pl0000* p,void*) {
    if(nested)return launcher?launcher(p):0;
    nested=true;uintptr_t result=0;
    __try {
        StealthStanceController::Get().BeginNativeWeaponRelease(p);
        if(launcher)result=launcher(p);
    }
    __finally {StealthStanceController::Get().EndNativeWeaponRelease(p);nested=false;}
    return result;
}
inline uintptr_t __fastcall Grenade(Pl0000* p,void*,float aim,float power) {
    // Retail 7A4410 reads two float arguments at EBP+8 / EBP+C and RET 8.
    // Forward both exactly; a no-argument trampoline corrupts throw behavior.
    if(nested)return grenade?grenade(p,aim,power):0;
    nested=true;uintptr_t result=0;
    __try {
        StealthStanceController::Get().BeginNativeWeaponRelease(p);
        if(grenade)result=grenade(p,aim,power);
    }
    __finally {StealthStanceController::Get().EndNativeWeaponRelease(p);nested=false;}
    return result;
}
inline void Install() {
    const unsigned char a[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x81,0xEC,0xC4,0x03,0,0};
    const unsigned char b[]={0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x81,0xEC,0xD4,0x03,0,0};
    if(std::memcmp(reinterpret_cast<void*>(shared::base+0x7A3EF0),a,sizeof(a)) ||
        std::memcmp(reinterpret_cast<void*>(shared::base+0x7A4410),b,sizeof(b)) ||
        *reinterpret_cast<unsigned char*>(shared::base+0x7A4406)!=0xC3 ||
        *reinterpret_cast<unsigned char*>(shared::base+0x7A48B4)!=0xC2 ||
        *reinterpret_cast<unsigned short*>(shared::base+0x7A48B5)!=8) {
        GunRuntimeLog::Write("[StealthWeapons] unsupported native release functions; keep native weapon pose");return;
    }
    static SafeHook::Hook first(reinterpret_cast<void*>(shared::base+0x7A3EF0),reinterpret_cast<void*>(Launcher),true,reinterpret_cast<void**>(&launcher));
    static SafeHook::Hook second(reinterpret_cast<void*>(shared::base+0x7A4410),reinterpret_cast<void*>(Grenade),true,reinterpret_cast<void**>(&grenade));
    installed=launcher && grenade;StealthStanceController::Get().SetNativeWeaponReleaseReady(installed);
    GunRuntimeLog::Write("[StealthWeapons] native aim/release pose hooks installed=%u",unsigned(installed));
}
}
