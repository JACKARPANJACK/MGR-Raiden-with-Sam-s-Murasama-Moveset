#pragma once

namespace SaiMovesetPolicy {
    // Native entry actions verified against Pl0000's dispatch table.
    constexpr unsigned StabbingDespair=0x9C, ForceOfMagnetism=0x9D;
    constexpr unsigned ThrowRecovery=0x9F, ForceRecovery=0xA3;
    constexpr unsigned NativeAirHeavy=0x9E;
    // Registered in SaiMovesetController::HandleAction, outside native dispatch.
    constexpr unsigned DoubleThrowStab=0x160, SpinSlice=0x161;
    constexpr unsigned Sommersault=0x162, UnavoidableFate=0x163;
    constexpr unsigned BufferTicks=24;
    struct Move { unsigned action, menu, native, motion; const char* name; };
    inline constexpr Move Moves[]={
        {DoubleThrowStab,36,0x9C,5,"Double Throw Stab"},
        {SpinSlice,38,0xA1,10,"Spin Slice"},
        {Sommersault,39,0xA2,22,"Sommersault"},
        {UnavoidableFate,40,0x9E,14,"Unavoidable Fate"}
    };
    inline const Move* Find(unsigned action) {
        for(const auto& move:Moves)if(move.action==action)return &move;
        return nullptr;
    }
    // These are indices in wp2030's MOT table, NOT animation-map/menu IDs.
    inline unsigned Motion(unsigned action,unsigned index) {
        if(action==DoubleThrowStab && index<=2)return index+5; // 2b10/11/12
        if(action==Sommersault) {
            if(index==19)return 22; // unused 2b75 somersault startup
            if(index==20 || index==26)return 27; // 2b7a landing
        }
        return index;
    }
    inline bool GroundAttack(unsigned state) {
        return (state>=0x9C && state<=0xA5 && state!=0x9E) ||
            state==DoubleThrowStab || state==SpinSlice || state==Sommersault;
    }
    inline bool GroundRecovery(unsigned state) {
        return state==ThrowRecovery || state==ForceRecovery || state==0xA0 ||
            state==0xA4 || state==0xA5 || state==DoubleThrowStab ||
            state==SpinSlice || state==Sommersault;
    }
    inline float CancelFrame(unsigned state) {
        return state==DoubleThrowStab ? 48.0f : Find(state) ? 36.0f :
            state==StabbingDespair || state==ForceOfMagnetism || state==NativeAirHeavy ? 20.0f : 8.0f;
    }
    inline bool RecoveryReady(unsigned state,bool initialized,float frame) {
        return GroundRecovery(state) && initialized && frame>=CancelFrame(state);
    }
    inline unsigned FollowUp(unsigned state,bool airborne) {
        // Spin Slice leaves the ground itself; airborne does not break YYY.
        if(state==SpinSlice)return Sommersault;
        if(state==NativeAirHeavy && airborne)return UnavoidableFate;
        if(airborne)return 0;
        switch(state) {
        case StabbingDespair: case ThrowRecovery: return DoubleThrowStab;
        case ForceOfMagnetism: case ForceRecovery: case DoubleThrowStab: return SpinSlice;
        default:return 0;
        }
    }
    inline unsigned AirAttack(unsigned state,bool airborne) {
        return airborne && (state==9 || state==0xA || state==0xB) ? UnavoidableFate : 0;
    }
    struct HeavyChain {
        unsigned presses=0;
        void Reset(){presses=0;}
        static bool Context(unsigned state) {
            return GroundAttack(state);
        }
        bool Poll(unsigned state,bool heavy,bool light) {
            if(light){Reset();return false;}
            // Idle marks a new string; damage and unrelated actions break it.
            // Native ground startup/recovery keeps the count intact; air has
            // its own first-native-strike prerequisite.
            if(!Context(state))Reset();
            if(heavy && (Context(state) || state<=4) && presses<3)++presses;
            return presses>=3;
        }
    };
    struct Buffer {
        unsigned state=0,ticks=0,next=0;
        void Reset(){state=ticks=next=0;}
        unsigned Poll(unsigned action,bool airborne,bool heavy,bool light,bool initialized,float frame,bool frozen=false) {
            const unsigned follow=FollowUp(action,airborne);
            const unsigned air=AirAttack(action,airborne);
            const unsigned target=follow?follow:air;
            const bool airContinuation=next==UnavoidableFate && target==UnavoidableFate;
            if(light || !target || (state && state!=action && !airContinuation))Reset();
            if(light)return 0;
            if(heavy && target){
                state=action;next=target;ticks=BufferTicks;
                const float remaining=air?0:CancelFrame(action)-frame;
                if(remaining>0)ticks+=unsigned(remaining)+1;
            }
            if(frozen)return 0;
            if(!ticks)return 0;
            if(air || (initialized && frame>=CancelFrame(action))){const unsigned result=next;Reset();return result;}
            if(--ticks==0)Reset();
            return 0;
        }
    };
    struct AirChain {
        bool nativeSeen=false,spent=false;
        void Reset(){nativeSeen=spent=false;}
        static bool Context(unsigned state) {
            return state==NativeAirHeavy || state==UnavoidableFate || state==9 || state==0xA || state==0xB;
        }
        bool Observe(unsigned state,bool airborne,bool initialized,bool light) {
            if(!airborne || light || !Context(state)){Reset();return false;}
            // Ground taps and merely jumping cannot unlock the added strike.
            // Observe the initialized regular aerial attack first.
            if(state==NativeAirHeavy && initialized && !spent)nativeSeen=true;
            return nativeSeen && !spent;
        }
        void Consume(){nativeSeen=false;spent=true;}
    };
    struct AerialLift {
        static constexpr unsigned SupportTicks=36,MaxAttackTicks=150;
        static constexpr float LaunchSpeed=0.18f,Acceleration=0.012f;
        static constexpr float MaxSpeed=0.22f,MaxRise=2.4f;
        float startHeight=0;
        bool active=false;
        void Start(float height){startHeight=height;active=true;}
        void Reset(){active=false;}
        bool Supports(unsigned ticks,unsigned phase,float height) const {
            return active && ticks<SupportTicks && phase<=1 && height<startHeight+MaxRise;
        }
        float Velocity(float current,unsigned ticks,unsigned phase,float height) const {
            if(!Supports(ticks,phase,height) || current>MaxSpeed)return current;
            float next=current+Acceleration;
            const float floor=ticks==0?LaunchSpeed:ticks<8?0.12f:0.025f;
            if(next<floor)next=floor;
            return next>MaxSpeed?MaxSpeed:next;
        }
        static bool Landed(unsigned native,unsigned phase,bool airborne) {
            return native==NativeAirHeavy && phase==3 && !airborne;
        }
    };
    // Preserve a custom identity while its native handler advances subphases,
    // projectiles, collision and recovery. Damage/idle exits are never swallowed.
    struct Machine {
        unsigned action=0,native=0,ticks=0;
        bool Enter(unsigned next) {
            const auto* move=Find(next);
            if(!move)return false;
            action=next;native=move->native;ticks=0;return true;
        }
        void Reset(){action=native=ticks=0;}
        static bool NativeRecovery(unsigned state) {
            return state==0x9F || state==0xA0 || state==0xA3 || state==0xA4 || state==0xA5;
        }
        bool Recovering(unsigned phase) const {
            return (NativeRecovery(native) && phase>=1) || (native==0xA2 && phase==7) ||
                (native==NativeAirHeavy && phase==5);
        }
        bool Finished(unsigned phase,float remaining) const {
            // These native handlers rely on input selection for their final
            // idle transition. Custom input ownership must provide that exit.
            return (remaining<=0 && Recovering(phase)) ||
                (action==UnavoidableFate && ticks>=AerialLift::MaxAttackTicks);
        }
        unsigned Complete(unsigned result) {
            ++ticks;
            if(result==native || NativeRecovery(result)) {
                native=result;return action;
            }
            Reset();return result;
        }
    };
}
