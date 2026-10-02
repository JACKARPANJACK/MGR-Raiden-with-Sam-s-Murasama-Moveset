#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include "GunChargePolicy.h"

namespace SmgPolicy
{
    // 20046 is a modeled enemy projectile. The native BulletBase descriptor
    // defaults to wpb000, a mesh-free bullet prefab (1CF32).
    constexpr unsigned Gun=0x30020, Bullet=0x3B000, MotionBank=0x10010;
    constexpr unsigned Muzzle=0x300, MuzzleEffect=0;
    constexpr unsigned EnemyMotionBank=0x20040;
    constexpr unsigned Attack=0xDF, LaunchAttack=0xDE;
    constexpr unsigned Interval=4, Magazine=30, Reload=90, Lifetime=120, MaxShots=40;
    constexpr unsigned DumpInterval=2;
    constexpr unsigned ReadyHold=180, RaiseTicks=16;
    constexpr const char* ReadyMotion="0400";
    constexpr const char* RaiseMotion="2200";
    struct ReadyWindow
    {
        unsigned remaining=0;
        void Fired() {remaining=ReadyHold;}
        void Tick(bool running,bool neutral) {if(!neutral) remaining=0;else if(running && remaining) --remaining;}
        void Cancel() {remaining=0;}
    };
    constexpr float Range=35.0f;
    enum Move { Burst, Launcher, Aerial, Sweep, Charged,FullCharged };
    inline bool Magdump(Move move) {return move==Charged || move==FullCharged;}
    inline bool Rising(Move move) {return move==Launcher;}
    inline int Damage(Move move) { return move==Launcher ? 6 : 4; }
    inline unsigned Cost(Move) {return 1;}
    inline const char* Motion(Move move,unsigned combo)
    {
        if(move==FullCharged) return "2578";
        if(move==Charged) return "2577";
        if(move==Launcher) return "2576";
        if(move==Aerial) return "2574";
        if(move==Sweep) return "2575";
        constexpr const char* codes[]={"2570","2571","2574","2575","2576"};
        return codes[combo%5];
    }
    inline unsigned MotionRelease(const char* code)
    {return !std::strcmp(code,"2570")?50:!std::strcmp(code,"2571")?49:
        !std::strcmp(code,"2577")?68:!std::strcmp(code,"2578")?59:41;}
    inline Move RoundMove(Move move,unsigned ordinal)
    {return Magdump(move)?(ordinal==0?Launcher:Burst):move==Burst && ordinal%3==2 ? Launcher : move;}
    struct FireControl
    {
        unsigned delay=0, rounds=Magazine, reload=0, queued=0, ordinal=0;
        Move move=Burst;
        GunChargePolicy::Hold charge;
        unsigned combo=0,chainWindow=0,chainStep=0;
        bool comboPose=false;
        void Cancel() { queued=0;charge.Cancel();chainWindow=chainStep=0; }
        void Tick(bool held,bool active,Move requested)
        {
            if(!active) { Cancel(); return; }
            if(delay) --delay;
            if(chainWindow && --chainWindow==0) chainStep=0;
            if(reload && --reload==0) rounds=Magazine;
            if(!charge.down && held && !queued && !reload) move=requested;
            if(charge.Tick(held,true,!queued && !reload) && !queued && !reload)
            {
                const unsigned tier=GunChargePolicy::Tier(charge.releasedFrames);
                if(tier) move=tier==2?FullCharged:Charged;
                // Repeated normal taps stay on enemy shooting poses. Special
                // Raiden clips require an explicit heavy/sweep/charge action.
                comboPose=move==Launcher || Magdump(move);
                if(move==Launcher) {if(chainWindow) ++chainStep;else chainStep=1;chainWindow=90;}
                queued=Magdump(move)?Magazine:move==Burst || move==Aerial || move==Sweep?3:1;
                // A charged request always dumps a full magazine. If partially
                // spent, reload first while retaining this one release request.
                if(Magdump(move) && rounds<Magazine) reload=Reload;
                ordinal=0;++combo;
            }
        }
        unsigned AnimationBank() const {return comboPose?MotionBank:EnemyMotionBank;}
        const char* AnimationCode() const
        {return comboPose?Motion(move==Launcher?Burst:move,chainStep?chainStep-1:0):"2400";}
        bool Ready() const { return queued && rounds>=Cost(move) && !delay && !reload; }
        void Fired()
        {
            if(!Ready()) return;
            --queued;rounds-=Cost(move);++ordinal;delay=Magdump(move)?DumpInterval:Interval;
            if(!rounds || (queued && rounds<Cost(move))) {reload=Reload;queued=0;}
        }
        void RejectUnfunded() {if(queued && !reload && rounds<Cost(move)) {reload=Reload;queued=0;}}
    };
    inline void Configure(unsigned char* data,Move move)
    {
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        unsigned flags=0;std::memcpy(&flags,data,4);
        word(0,flags|4);word(4,Bullet);
        // Keep the enemy gun's collision type and attack data, with the generic
        // native bullet prefab instead of its modeled enemy ammunition.
        word(0x110,0x58);word(0x114,0x34);
        word(0x10,Rising(move)?LaunchAttack:Attack);word(0x14,Damage(move));
        word(0x18,0);word(0x1C,15);
    }
    inline bool OwnedHit(unsigned attack,int damage,bool owned)
    {return owned && ((attack==Attack && damage==4) || (attack==LaunchAttack && damage==6));}
}
