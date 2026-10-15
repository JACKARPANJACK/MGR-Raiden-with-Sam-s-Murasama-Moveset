#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include "GunChargePolicy.h"
#include "SamDirectionalPolicy.h"

namespace SmgPolicy
{
    constexpr unsigned Buttons(unsigned binding) {return binding&~0xFu;}
    constexpr unsigned ControllerAim=0x400u,ControllerFire=0x4000u; // LB, RT.
    constexpr bool ControllerAimHeld(unsigned buttons) {return (buttons&ControllerAim)!=0;}
    inline bool ControllerFireHeld(unsigned buttons,float trigger)
    {return (buttons&ControllerFire)!=0 || (std::isfinite(trigger) && trigger>.2f);}
    inline bool BodyMotionCode(const char* name,char (&code)[5])
    {
        code[0]=0;
        if(!name) return false;
        if(!std::strncmp(name,"pl0010_",7) || !std::strncmp(name,"pl0000_",7)) name+=7;
        const size_t length=std::strlen(name);
        if(length!=4 && !(length==8 && !std::strcmp(name+4,".mot"))) return false;
        for(unsigned i=0;i<4;++i)
            if(!((name[i]>='0' && name[i]<='9') || (name[i]>='a' && name[i]<='f'))) return false;
        std::memcpy(code,name,4);code[4]=0;return true;
    }
    inline bool SlideClip(const char* name)
    {
        char code[5];if(!BodyMotionCode(name,code)) return false;
        return !std::strcmp(code,"0030") || !std::strcmp(code,"0031") || !std::strcmp(code,"0032") ||
            !std::strcmp(code,"4230") || !std::strcmp(code,"4231") || !std::strcmp(code,"4232") ||
            !std::strcmp(code,"9630") || !std::strcmp(code,"9631") || !std::strcmp(code,"9632");
    }
    inline bool MeleeClip(const char* name)
    {
        char code[5];if(!BodyMotionCode(name,code)) return false;
        // 2eXX are the rifle donor clips; 257X are native subweapon clips.
        if(code[0]=='2') return code[1]!='e' && std::strncmp(code,"257",3)!=0;
        if(!std::strncmp(code,"350",3) || !std::strncmp(code,"353",3) || !std::strncmp(code,"354",3)) return true;
        return !std::strncmp(code,"900",3) || !std::strncmp(code,"903",3) ||
            !std::strncmp(code,"904",3) || !std::strncmp(code,"910",3) ||
            !std::strncmp(code,"911",3) || !std::strncmp(code,"94c",3) || !std::strncmp(code,"941",3);
    }
    inline bool ParkourClip(const char* name)
    {
        char code[5];if(!BodyMotionCode(name,code)) return false;
        // Native wall-run / wall-slide families, including MG RAY traversal.
        return !std::strncmp(code,"60",2) || !std::strncmp(code,"d15",3);
    }
    constexpr bool GunOverlay(bool slide,bool parkour,bool airborne,bool locomotion,bool ninjaRun)
    {return !slide && !parkour && (airborne || locomotion || ninjaRun);}
    struct AimControl
    {
        static constexpr unsigned HoldTicks=10; // Maximum duration of a quick-fire tap.
        unsigned frames=0;
        bool down=false,aiming=false,requireRelease=false,triggered=false,tapped=false;
        void Cancel() {frames=0;down=aiming=triggered=tapped=false;requireRelease=true;}
        bool Tick(bool aim,bool trigger,bool enabled,bool slide,bool lock=false)
        {
            tapped=false;
            if(!enabled || slide) {Cancel();if(!aim) requireRelease=false;return false;}
            if(!aim)
            {
                const bool tap=down && frames<HoldTicks && !triggered && !requireRelease;
                frames=0;down=aiming=requireRelease=triggered=false;
                tapped=tap;
                return tap;
            }
            if(requireRelease) return false;
            down=true;
            if(frames<HoldTicks) ++frames;
            aiming=!lock && (frames>=HoldTicks || trigger);
            // Lock + aim is a firing chord; releasing it must not add a tap.
            triggered=triggered || trigger || lock;
            return lock || (aiming && trigger);
        }
    };
    struct SprayGesture
    {
        SamDirectionalPolicy::Input direction;
        bool active=false;
        void Reset() {direction.Reset();active=false;}
        bool Tick(float x,float y,bool held)
        {
            direction.Poll(x,y,false,false);
            if(!held) active=false;
            else if(direction.flickTicks && direction.flick!=SamDirectionalPolicy::None)
            {
                active=true;direction.flickTicks=0;direction.flick=SamDirectionalPolicy::None;
            }
            return held && active;
        }
    };
    // Basic rifle cyborg Em0010: wp0040 geometry, wp004f effect/sound bank.
    // Its rifle descriptor retains the constructor's generic wpb000 prefab.
    constexpr unsigned Gun=0x30040, GunBank=0x3004F, Bullet=0x3B000, MotionBank=0x10010;
    constexpr unsigned BulletPool=0x4B000;
    constexpr unsigned Muzzle=0, MuzzleEffect=0;
    constexpr const char* GunContext="Em0010_Assault";
    constexpr float BulletSpeed=100.0f, TrajectoryScale=0.8f;
    constexpr float MuzzleUp=0.01f, MuzzleForward=0.39f;
    constexpr unsigned EnemyMotionBank=MotionBank; // All gun clips are retargeted into Pl0010.
    constexpr unsigned Attack=0x146, LaunchAttack=0x146;
    constexpr unsigned Interval=4, Magazine=30, Reload=90, Lifetime=120, MaxShots=40;
    constexpr unsigned DumpInterval=2;
    constexpr unsigned ReadyHold=180, RaiseTicks=21;
    constexpr const char* IdleMotion="2e00"; // Em0010 2400: relaxed, lowered rifle stance
    constexpr const char* ReadyMotion="2e03"; // Em0010 2402: level firing stance
    constexpr const char* RaiseMotion="2e01";
    constexpr const char* RecoilMotion="2e03"; // 2404 is upward aim, not recoil
    constexpr const char* ReloadMotion="2e06";
    constexpr const char* WalkStartMotion="2e10";
    constexpr const char* WalkLoopMotion="2e11";
    constexpr const char* WalkStopMotion="2e12";
    constexpr const char* WalkStopAltMotion="2e13";
    constexpr const char* WalkBackMotion="2e15";
    constexpr const char* WalkLeftMotion="2e16";
    constexpr const char* WalkRightMotion="2e17";
    constexpr const char* RunStartMotion="2e20";
    constexpr const char* RunLoopMotion="2e20";
    constexpr const char* RunStopMotion="2e22";
    constexpr const char* JogLoopMotion="2e21";
    constexpr const char* RunAltMotion="2e23";

    inline const char* Resolve(const char* name)
    {
        if(!name) return nullptr;
        const char* code=name;
        if(!std::strncmp(code,"pl0010_",7) || !std::strncmp(code,"pl0000_",7)) code+=7;
        const size_t len=std::strlen(code);
        if(len!=4 && !(len==8 && !std::strcmp(code+4,".mot"))) return nullptr;

        // Idle / Combat idle: 0000, 0200, 0201, 0202, 0401 -> 2e00
        if(!std::strncmp(code,"0000",4) || !std::strncmp(code,"0200",4) ||
           !std::strncmp(code,"0201",4) || !std::strncmp(code,"0202",4) ||
           !std::strncmp(code,"0401",4))
            return IdleMotion;

        // Enemy body clips with the exact requested player root/timing.
        constexpr const char* player[]={"0010","0011","0012","0013","0020","0021","0022","0023",
                                        "0210","0211","0212","0213","0220","0221","0222","0223"};
        constexpr const char* motion[]={"2e40","2e41","2e42","2e43","2e44","2e45","2e46","2e47",
                                        "2e48","2e49","2e4a","2e4b","2e4c","2e4d","2e4e","2e4f"};
        for(unsigned i=0;i<16;++i) if(!std::strncmp(code,player[i],4)) return motion[i];

        return nullptr;
    }
    inline bool Stationary(float inputMagnitudeSquared,float stickX,float stickY,bool keyboardMove,float speedX,float speedZ)
    {
        return !keyboardMove && inputMagnitudeSquared<0.0001f &&
            stickX*stickX+stickY*stickY<0.01f && std::abs(speedX)+std::abs(speedZ)<0.003f;
    }
    constexpr unsigned FireInputMask(unsigned fire,unsigned sprint,unsigned jump,unsigned light,unsigned heavy,unsigned blade)
    { return fire & ~(0xFu | sprint | jump | light | heavy | blade); }
    struct ReadyWindow
    {
        unsigned remaining=0;
        void Fired() {remaining=ReadyHold;}
        void Tick(bool running,bool neutral) {if(!neutral) remaining=0;else if(running && remaining) --remaining;}
        void Cancel() {remaining=0;}
    };
    constexpr float Range=35.0f;
    inline bool OnScreen(bool projected,float depth,float x,float y,float width,float height)
    {
        return projected && std::isfinite(depth+x+y+width+height) && depth>.05f &&
            width>0 && height>0 && x>=0 && x<=width && y>=0 && y<=height;
    }
    inline unsigned SprayIndex(unsigned shot,unsigned count)
    {
        if(count<2) return 0;
        const unsigned step=shot%(2*(count-1));
        return step<count?step:2*(count-1)-step;
    }
    inline float StrideRate(float speed,bool running)
    {return (std::clamp)(speed/(running?.18f:.065f),.35f,2.5f);}
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
    inline Move RoundMove(Move move,unsigned) {return move==Aerial?Aerial:move==Sweep?Sweep:Burst;}
    inline bool Spray(Move move,bool wideInput) {return wideInput || move==Sweep || Magdump(move);}
    constexpr float SprayWidth=0.30f, SprayHeight=0.10f;
    inline float JuggleSpeed(float speed) {return (std::max)(speed,0.10f);}
    inline float AirComboSpeed(float speed) {return (std::max)(speed,-0.025f);}
    struct FireControl
    {
        unsigned delay=0, rounds=Magazine, reload=0, queued=0, ordinal=0,burstRemaining=0;
        Move move=Burst;
        GunChargePolicy::Hold charge;
        unsigned combo=0,chainWindow=0,chainStep=0;
        bool comboPose=false;
        void Cancel() { queued=burstRemaining=0;charge.Cancel();chainWindow=chainStep=0; }
        void Tick(bool held,bool active,Move requested,bool burst=false)
        {
            if(!active) {Cancel();return;}
            if(delay) --delay;
            if(reload && --reload==0) rounds=Magazine;
            // Automatic fire starts on press, not release. Cancellation still
            // requires release before rearming after menus/story interruptions.
            charge.Tick(held,true,true);
            if(burst && !charge.requireRelease && !reload)
            {burstRemaining=3;move=requested;}
            const bool firing=held && !charge.requireRelease;
            queued=(firing || burstRemaining)?1u:0u;
            if(firing) move=requested;
            comboPose=false;
        }
        unsigned AnimationBank() const {return MotionBank;}
        const char* AnimationCode() const
        {return RecoilMotion;}
        bool SummonRequested(bool held) const {return !reload && held && !charge.requireRelease;}
        bool Ready() const { return queued && rounds>=Cost(move) && !delay && !reload; }
        void Fired()
        {
            if(!Ready()) return;
            --queued;rounds-=Cost(move);++ordinal;delay=Interval;
            if(burstRemaining) --burstRemaining;
            if(!rounds || (queued && rounds<Cost(move))) {reload=Reload;burstRemaining=0;}
        }
        void RejectUnfunded() {if(queued && !reload && rounds<Cost(move)) reload=Reload;}
    };
    inline void Configure(unsigned char* data,Move move)
    {
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        unsigned flags=0;std::memcpy(&flags,data,4);
        word(0,flags|4);word(4,Bullet);
        // Em0010 rifle descriptor (73839A..7384DB): native attack 146,
        // 7383AC temporarily subtracts 8 from ESP before the write at 7383E5:
        // [ESP+168] is descriptor+110, not descriptor+114. FFFF disables it.
        word(0x110,0x19);word(0x114,0);
        const float drag=0.998f;
        std::memcpy(data+0x16C,&drag,4);
        word(0x10,Rising(move)?LaunchAttack:Attack);word(0x14,Damage(move));
        word(0x18,0);word(0x1C,15);
    }
    inline bool OwnedHit(unsigned attack,int damage,bool owned)
    {return owned && ((attack==Attack && damage==4) || (attack==LaunchAttack && damage==6));}
}
