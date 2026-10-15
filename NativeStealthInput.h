#pragma once
#include "StealthStanceController.h"
#include "StealthCqcController.h"
#include "MonsoonKickController.h"
// Native virtual slot 0x3C4 dispatches after the engine collects CurrentInput.
// Filtering only at TickGame entry lets that collection overwrite the mask.
namespace NativeStealthInput {
using Dispatch=void(__thiscall*)(Pl0000*);
inline Dispatch raiden=nullptr,sam=nullptr;
inline bool installed=false;
inline void __fastcall Raiden(Pl0000* p,void*) {
    StealthStanceController::Get().FilterNativeInput(p);
    StealthCqcController::Get().SampleNativeInput(p);
    MonsoonKickController::Get().SampleNativeInput(p);
    if(raiden)raiden(p);
    MonsoonKickController::Get().AfterNativeInput(p);
    StealthCqcController::Get().AfterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
}
inline void __fastcall Sam(Pl0000* p,void*) {
    StealthStanceController::Get().FilterNativeInput(p);
    StealthCqcController::Get().SampleNativeInput(p);
    if(sam)sam(p);
    StealthCqcController::Get().AfterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
}
inline void Install() {
    const unsigned char a[]={0x83,0xEC,0x08,0xF6,0x05};
    const unsigned char b[]={0x51,0x56,0x57,0x8B,0xF1,0x33,0xFF};
    if(std::memcmp(reinterpret_cast<void*>(shared::base+0x8104B0),a,sizeof(a)) ||
        *reinterpret_cast<unsigned*>(shared::base+0x8104B5)!=shared::base+0x17EA090 ||
        *reinterpret_cast<unsigned char*>(shared::base+0x8104B9)!=0x20 ||
        std::memcmp(reinterpret_cast<void*>(shared::base+0x492EB0),b,sizeof(b))) {
        GunRuntimeLog::Write("[StealthInput] unsupported native input dispatch; hooks disabled");return;
    }
    static SafeHook::Hook first(reinterpret_cast<void*>(shared::base+0x8104B0),reinterpret_cast<void*>(Raiden),true,reinterpret_cast<void**>(&raiden));
    static SafeHook::Hook second(reinterpret_cast<void*>(shared::base+0x492EB0),reinterpret_cast<void*>(Sam),true,reinterpret_cast<void**>(&sam));
    installed=raiden && sam;
    GunRuntimeLog::Write("[StealthInput] native Raiden/Sam input filters installed=%u",unsigned(installed));
}
}
