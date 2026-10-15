#pragma once
#include <cmath>
#include "SaiPullPolicy.h"
#include "SamDirectionalPolicy.h"
namespace StealthCqcPolicy {
constexpr unsigned Action=0x100079;
enum class Move { None, Combo, Grab, Hold, Throw, Knife, Rise };
inline bool Unarmed(int equipped,int swordState,bool sam,bool roundTrip){return equipped==5 && swordState==1 && !sam && !roundTrip;}
constexpr unsigned TakedownButton=0x20; // Controller B, reserved only for Unarmed.
enum class Command { Native, Combo, Grab, Throw, Knife, Release, Takedown };
inline Command Resolve(SamDirectionalPolicy::Direction direction,bool light,bool heavy,bool holding,bool takedown) {
    using namespace SamDirectionalPolicy;
    if(takedown)return holding?Command::Native:Command::Takedown;
    if(light==heavy)return Command::Native;
    if(holding) {
        if(heavy && direction==Back)return Command::Knife;
        if(heavy && direction==None)return Command::Release;
        if(light && direction!=None)return Command::Throw;
        if(light)return Command::Combo;
        return Command::Native;
    }
    if(heavy && direction==Forward)return Command::Grab;
    if(light && direction==Forward)return Command::Combo;
    return Command::Native;
}
// AF/B0 are wp2040's ordinary light/heavy combo handlers (7A7060/7A7290).
// Their player animation-name slot can be empty, so action detection is required.
inline bool NativeAttackAction(unsigned action){return action>=0xA8 && action<=0xB0;}
// AF increments 2620 before playback: punches 1/2/3/4 leave 1/2/3/0.
// 940 is temporary animation scratch and is cleared again during playback.
inline bool NativeLightFinisher(unsigned action,unsigned phase,unsigned nextPunch){return action==0xAF && phase!=0 && nextPunch==0;}
// Native 2c03's final hit window ends at frame 37. Blend to the extension
// after six recovery frames, before its long return-to-idle tail at frame 80.
constexpr float NativeLightRecoveryFrame=43;
inline Command ExtendNativePunch(Command command,bool finisher,SamDirectionalPolicy::Direction direction,bool light,bool heavy) {
    return command==Command::Native && finisher && direction==SamDirectionalPolicy::None && light && !heavy ? Command::Combo : command;
}
inline bool NativeAttackClip(const char* code) {
    if(!code || code[0]!='2' || (code[1]!='c' && code[1]!='C'))return false;
    const auto hex=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;};
    const int a=hex(code[2]),b=hex(code[3]);if(a<0 || b<0)return false;
    const int n=a*16+b;
    return n<=0x03 || (n>=0x10&&n<=0x12) || n==0x20 || n==0x21 || (n>=0x24&&n<=0x2E) || (n>=0x30&&n<=0x33) ||
        n==0x40 || n==0x41 || n==0x50 || n==0x51 || n==0x60 || (n>=0x65&&n<=0x68) || (n>=0x6A&&n<=0x6D);
}
struct Buffer {
    Command command=Command::Native;unsigned ticks=0,direction=0;
    void Clear(){command=Command::Native;ticks=direction=0;}
    // wp2040's native light finisher lasts 121 source frames. An early press
    // must survive until recovery rather than expiring after 36 updates.
    static constexpr unsigned Lifetime=180;
    void Set(Command next,unsigned dir){command=next;direction=dir;ticks=Lifetime;}
    bool Active()const{return command!=Command::Native && ticks>0;}
    void Tick(bool safe){if(!safe || (ticks && !--ticks))Clear();}
    Command Take(bool ready){if(!ready || !Active())return Command::Native;const auto next=command;Clear();return next;}
};
// wp2040 sequences expose their native combo/return windows through bits 1/17.
// The body node's remaining-time query alone misses weapon-owned AF/B0 clips.
inline bool Recovery(bool nativeAttack,float remaining,unsigned sequenceFlags=0) {
    return nativeAttack && ((sequenceFlags&0x20002u)!=0 || (std::isfinite(remaining) && remaining>=0 && remaining<=8));
}
struct Playback {
    unsigned length=0;
    bool requestTick=true;
    float frame=0;
    void Start(unsigned frames){length=frames;requestTick=true;frame=0;}
    bool Observe(float nativeFrame) {
        // RequestAnimationByName changes the slot name synchronously, but the
        // actor frame still belongs to the prior clip until the outer update.
        if(requestTick){requestTick=false;return false;}
        if(length<3 || !std::isfinite(nativeFrame) || nativeFrame<0)return false;
        frame=nativeFrame;return true;
    }
    // Live native playback clamps a count-86 MOT at frame 84, not 85.
    bool Finished()const{return length>=3 && frame>=float(length-2);}
};
inline bool ChainReady(bool queued,unsigned combo,float frame,float recovery) {
    return queued && combo<3 && std::isfinite(frame+recovery) && recovery>0 && frame>=recovery;
}
inline bool ComboRecovered(float frame,float recovery) {
    return std::isfinite(frame+recovery) && recovery>0 && frame>=recovery;
}
inline unsigned ThrowDirection(SamDirectionalPolicy::Direction direction) {
    using namespace SamDirectionalPolicy;
    return direction==Back?1u:direction==Left?2u:direction==Right?3u:0u;
}
inline bool Eligible(unsigned id) {return SaiPullPolicy::Eligible(id);}
inline bool Reach(float dx,float dy,float dz,float yaw,float range=1.65f) {
    return std::isfinite(dx+dy+dz+yaw) && std::fabs(dy)<.8f && dx*dx+dz*dz<=range*range &&
        dx*std::sin(yaw)+dz*std::cos(yaw)>.05f;
}
inline bool Rear(float dx,float dz,float victimYaw,int caution) {
    const float distance=std::hypot(dx,dz);
    return distance>.05f && caution<3 && (dx*std::sin(victimYaw)+dz*std::cos(victimYaw))/distance>.65f;
}
inline int Clip(Move move,bool low,unsigned combo=0,unsigned direction=0) {
    switch(move) {
    case Move::Rise:return 32;
    case Move::Combo:return 18+int(combo%4);
    case Move::Grab:return low?24:22;
    case Move::Hold:return low?25:23;
    case Move::Throw:return 26+int(direction%4);
    case Move::Knife:return low?31:30;
    default:return -1;
    }
}
}
