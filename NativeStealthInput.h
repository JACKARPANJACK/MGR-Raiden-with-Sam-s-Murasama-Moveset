#pragma once
#include "StealthStanceController.h"
#include "StealthCqcController.h"
#include "MonsoonKickController.h"
#include "KunaiSubweapon.h"
// Native virtual slot 0x3C4 dispatches after the engine collects CurrentInput.
// Filtering only at TickGame entry lets that collection overwrite the mask.
namespace NativeStealthInput {
using Dispatch=void(__thiscall*)(Pl0000*);
inline Dispatch raiden=nullptr,sam=nullptr;
inline bool installed=false;
using MouseUpdate=void(__cdecl*)(Hw::cMouseState&);
inline MouseUpdate mouseUpdate=nullptr;
inline void __cdecl GunMouseUpdate(Hw::cMouseState& state) {
    if(mouseUpdate)mouseUpdate(state);
    // The native input poll runs inside TickGame, after our early reservation.
    // Consume the newly polled LMB before keyboard bindings translate it to melee.
    if(&state!=&g_Mouse || !KunaiSubweapon::GetSmg().CapturesGunAim())return;
    state.m_On=SmgPolicy::FilterGunMouse(state.m_On,true);
    state.m_Trig=SmgPolicy::FilterGunMouse(state.m_Trig,true);
    state.m_Rel=SmgPolicy::FilterGunMouse(state.m_Rel,true);
    state.m_Rep=SmgPolicy::FilterGunMouse(state.m_Rep,true);
    static bool reported=false;
    if(!reported){reported=true;GunRuntimeLog::Write("[GunInput] post-poll LMB capture active");}
}
inline void InstallGunMouse() {
    // Absolute operand is image-base relative in memory.
    if(*reinterpret_cast<unsigned short*>(shared::base+0x9D9800)!=0x05DB ||
       *reinterpret_cast<unsigned*>(shared::base+0x9D9802)!=shared::base+0x14CDDFC) {
        GunRuntimeLog::Write("[GunInput] unsupported mouse poll; hook disabled");return;
    }
    static SafeHook::Hook mouse(reinterpret_cast<void*>(shared::base+0x9D9800),reinterpret_cast<void*>(GunMouseUpdate),true,reinterpret_cast<void**>(&mouseUpdate));
    GunRuntimeLog::Write("[GunInput] build=c-lmb-post-poll-1 mouse-hook=%u",unsigned(mouseUpdate!=nullptr));
}
inline void __fastcall Raiden(Pl0000* p,void*) {
    KunaiSubweapon::GetSmg().FilterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
    StealthCqcController::Get().SampleNativeInput(p);
    MonsoonKickController::Get().SampleNativeInput(p);
    SaiMovesetController::Get().SampleNativeInput(p);
    if(!SaiMovesetController::Get().OwnsNativeInput(p) && raiden)raiden(p);
    SaiMovesetController::Get().AfterNativeInput(p);
    KunaiSubweapon::GetSmg().FilterNativeInput(p);
    MonsoonKickController::Get().AfterNativeInput(p);
    StealthCqcController::Get().AfterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
}
inline void __fastcall Sam(Pl0000* p,void*) {
    KunaiSubweapon::GetSmg().FilterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
    StealthCqcController::Get().SampleNativeInput(p);
    if(sam)sam(p);
    KunaiSubweapon::GetSmg().FilterNativeInput(p);
    StealthCqcController::Get().AfterNativeInput(p);
    StealthStanceController::Get().FilterNativeInput(p);
}
inline void Install() {
    InstallGunMouse();
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
