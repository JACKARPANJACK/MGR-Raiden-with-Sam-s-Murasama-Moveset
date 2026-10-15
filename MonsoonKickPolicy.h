#pragma once
#include "SamDirectionalPolicy.h"
#include <atomic>
namespace MonsoonKickPolicy {
    inline std::atomic<const char*> Status{"Ready: Back + Heavy with Sai"};
    inline bool WeaponAllowed(int selected,int equipped,bool sam) {
        return !sam && selected==2 && equipped==3;
    }
    constexpr unsigned Action = 0x142; // beyond Raiden's native action dispatch table
    constexpr const char* Code = "2f00";
    constexpr float JumpCancelFrame = 54.0f;
    inline bool CanJumpCancel(float frame, bool jump) {return jump && frame>=JumpCancelFrame;}
    enum class FollowUp {None, Light, Heavy, Chord, Jump, Pull};
    inline FollowUp Command(bool light,bool heavy,bool jump,bool pull) {
        return pull ? FollowUp::Pull : jump ? FollowUp::Jump : light && heavy ? FollowUp::Chord :
            light ? FollowUp::Light : heavy ? FollowUp::Heavy : FollowUp::None;
    }
    struct CancelBuffer {
        FollowUp command=FollowUp::None;
        unsigned ticks=0;
        void Clear(){command=FollowUp::None;ticks=0;}
        FollowUp Poll(float frame,bool hit,FollowUp edge,bool frozen=false) {
            if(edge!=FollowUp::None){command=edge;ticks=12;}
            if(frozen || !ticks)return FollowUp::None;
            // Preserve the kick's first impact. A confirmed launch can cancel
            // immediately; misses must finish the native attack-track window.
            if(frame>=(hit ? 44.0f : 64.0f)){auto next=command;Clear();return next;}
            if(--ticks==0)Clear();
            return FollowUp::None;
        }
    };
    inline unsigned StartState(unsigned current, unsigned previous, bool initializing, bool sai=false) {
        // Native input selection can enter a sword attack before HandleActions.
        // Replace only its first update, when this Heavy press began from idle.
        const bool nativeAttack=sai && current>=0x9C && current<=0xA5;
        return initializing && previous<=4 && nativeAttack ? previous : current;
    }
    inline bool ConfirmedLaunch(int hpBefore, int hp, bool owned, float frame, bool launched) {
        return !launched && owned && hp>0 && hpBefore>hp && frame>=42 && frame<=64;
    }
    inline bool CanStart(unsigned state, bool alive, bool air, bool blade, bool blocked,
                         bool supportedWeapon, bool heavy, bool light, float x, float y) {
        return state <= 4 && alive && !air && !blade && !blocked && supportedWeapon &&
            heavy && !light && SamDirectionalPolicy::Resolve(x,y) == SamDirectionalPolicy::Back;
    }
}
