#pragma once
#include <BehaviorEmBase.h>
#include "StealthStanceController.h"
#include "StealthDetectionPolicy.h"

namespace NativeStealthDetection {
using Update=void(__thiscall*)(cEnemyCautionStateManager*);
inline Update original=nullptr;
inline bool installed=false;
inline thread_local bool nested=false;
inline void __fastcall Tick(cEnemyCautionStateManager* manager,void*) {
    const auto stance=StealthStanceController::Get().DetectionStance();
    Entity* entity=manager?manager->field_40.getEntity():nullptr;
    const bool ordinary=entity && entity->m_pBehavior && entity->isAlive() &&
        StealthDetectionPolicy::Reduce(unsigned(entity->m_ObjId),manager->m_nState,stance);
    // Confirm the native owner and embedded manager; bosses and other sensor
    // implementations must never be treated as ordinary cyborgs.
    if(nested || !ordinary || &static_cast<BehaviorEmBase*>(entity->m_pBehavior)->m_CautionStateManager!=manager) {
        if(original)original(manager);return;
    }
    constexpr unsigned offsets[]={0x24,0x2c,0x30,0x38};
    float saved[4]{},scaled[4]{};
    const int state=manager->m_nState;
    for(unsigned i=0;i<4;++i) {
        auto* slot=reinterpret_cast<float*>(reinterpret_cast<char*>(manager)+offsets[i]);
        saved[i]=*slot;scaled[i]=StealthDetectionPolicy::RangeSquared(saved[i],stance);*slot=scaled[i];
    }
    nested=true;
    __try {if(original)original(manager);}
    __finally {
        // Native transitions may have replaced the current parameter block.
        // Restore our temporary changes only; never undo a native transition.
        for(unsigned i=0;i<4;++i) {
            auto* slot=reinterpret_cast<float*>(reinterpret_cast<char*>(manager)+offsets[i]);
            *slot=StealthDetectionPolicy::RestoreRange(*slot,saved[i],scaled[i],state,manager->m_nState);
        }
        nested=false;
    }
}
inline void Install() {
    const unsigned char prefix[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0x84,0,0,0,0x53,0x56,0x8b,0xf1};
    const unsigned char primary[]={0xd8,0x56,0x30},secondary[]={0xd8,0x5e,0x24};
    if(std::memcmp(reinterpret_cast<void*>(shared::base+0x689560),prefix,sizeof(prefix)) ||
       std::memcmp(reinterpret_cast<void*>(shared::base+0x689A22),primary,sizeof(primary)) ||
       std::memcmp(reinterpret_cast<void*>(shared::base+0x689B1E),secondary,sizeof(secondary))) {
        GunRuntimeLog::Write("[Stealth] native perception signature mismatch; reduction disabled");return;
    }
    static SafeHook::Hook hook(reinterpret_cast<void*>(shared::base+0x689560),reinterpret_cast<void*>(Tick),true,reinterpret_cast<void**>(&original));
    installed=original!=nullptr;
    GunRuntimeLog::Write("[Stealth] native cyborg perception hook installed=%d",int(installed));
}
}
