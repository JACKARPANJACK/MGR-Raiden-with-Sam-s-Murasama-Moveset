#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace KunaiPolicy
{
    enum Variant { Native, Stun, Explosive, Heat, Heatblades, VectorSmg, Count };
    constexpr unsigned Object = 0x30372;
    constexpr unsigned GrenadeObject = 0x31011;
    constexpr unsigned Attack = 0x151;
    constexpr int GrenadeSlot = 1;
    constexpr unsigned MaxShots = 32, Lifetime = 300;
    constexpr unsigned StunCharge=18, ExplosiveCharge=45, VolleyCharge=90;
    constexpr unsigned VolleySize=10, HomingLifetime=120, FanHomingDelay=18;
    constexpr float FanHalfAngle=0.785398163f; // 90-degree horizontal fan
    inline float FanSpread(unsigned index,unsigned count)
    {return count>1 ? (2.0f*float(index)/float(count-1)-1.0f)*FanHalfAngle : 0;}
    constexpr unsigned RapidInterval=6, BurstSize=10, BurstCooldown=120;
    constexpr unsigned AimHold=6;
    inline bool PrecisionAim(unsigned frames) {return frames>=AimHold;}
    constexpr float ReticleX=0.5f, ReticleY=0.44f, ThrowBlend=0.0833333f;
    constexpr float AimPlayback=2.5f, ThrowPlayback=3.0f, ReleaseBlend=0.05f;
    inline unsigned PoseTicks(unsigned length,bool aiming)
    {return unsigned(std::ceil(float(length)/(aiming ? AimPlayback : ThrowPlayback)));}
    inline bool CanStartPose(bool active,bool activeAim,bool wantAim)
    {return !active || (!wantAim && activeAim);}
    struct ThrowPose
    {
        unsigned remaining=0;
        int node=-1, returnMap=4;
        void Begin(unsigned length,int handle,int previousMap)
        {
            if(remaining || handle<0) return;
            remaining=length;node=handle;returnMap=previousMap;
        }
        bool Tick(bool advance,bool owns,bool neutral)
        {
            if(!remaining || !advance) return false;
            if(!owns || !neutral) {remaining=0;node=-1;return false;}
            if(--remaining) return false;
            node=-1;return true;
        }
    };
    struct Burst
    {
        unsigned used=0, remaining=0;
        bool recovering=false;
        void Tick(bool advance)
        {
            if(advance && remaining && --remaining==0 && recovering)
            {used=0;recovering=false;}
        }
        unsigned Capacity() const {return remaining ? 0 : BurstSize-used;}
        void Fired(unsigned count,bool empty)
        {
            if(!count) return;
            used=(std::min)(BurstSize,used+count);
            recovering=used==BurstSize || empty;
            remaining=recovering ? BurstCooldown : RapidInterval;
        }
    };
    // A full-body release pose may replace neutral movement only. Attacks,
    // air movement and Blade Mode retain their animation while knives release.
    inline bool CanAnimateThrow(unsigned state,bool airborne,bool bladeMode)
    {return !airborne && !bladeMode && (state<=4 || (state>=0x100000 && state<=0x100002));}
    struct ReleasePlan { int variant; unsigned count; bool homing; };
    inline ReleasePlan Plan(unsigned frames,int selected,unsigned ammo,unsigned capacity)
    {
        int variant=frames>=ExplosiveCharge && frames<VolleyCharge ? Explosive :
            frames>=StunCharge && frames<ExplosiveCharge ? Stun : selected;
        const unsigned desired=frames>=VolleyCharge ? VolleySize : 1;
        return {variant,(std::min)(desired,(std::min)(ammo,capacity)),frames>=StunCharge};
    }
    struct Charge
    {
        bool holding=false, suppressed=false;
        unsigned frames=0;
        void Cancel(bool down) { holding=false; frames=0; suppressed=down; }
        // An interrupted hold must be released before it can start again.
        bool Tick(bool down,bool eligible,bool advance)
        {
            if (!eligible) { Cancel(down); return false; }
            if (suppressed) { if(!down) suppressed=false; return false; }
            if (down)
            {
                holding=true;
                if(advance && frames<VolleyCharge) ++frames;
                if(frames==VolleyCharge) {holding=false;suppressed=true;return true;}
                return false;
            }
            if (!holding) return false;
            holding=false; return true; // caller consumes frames, then clears them
        }
    };
    struct Vector { float x=0,y=0,z=0; };
    inline float Length(Vector v) { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
    inline Vector Unit(Vector v)
    {
        const float length=Length(v);
        return std::isfinite(length) && length>0.0001f ? Vector{v.x/length,v.y/length,v.z/length} : Vector{0,0,1};
    }
    inline float Dot(Vector a,Vector b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
    inline float AimDepth(Vector offset,Vector direction)
    {return std::clamp(Dot(offset,direction),1.0f,60.0f);}
    inline float AimScore(Vector offset,Vector direction,float range,float cone)
    {
        const float length=Length(offset);
        if (!std::isfinite(length) || length<0.1f || length>range) return -1;
        const float alignment=Dot(Unit(offset),direction);
        return alignment<cone ? -1 : (1-alignment)*1000+length*0.01f;
    }
    inline Vector Steer(Vector velocity,Vector offset)
    {
        const float speed=Length(velocity);
        const float distance=Length(offset);
        if (!std::isfinite(speed) || !std::isfinite(distance) || speed<0.0001f || distance<0.1f) return velocity;
        const auto current=Unit(velocity),desired=Unit(offset);
        auto cross=[](Vector a,Vector b) { return Vector{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; };
        const float angle=std::acos(std::clamp(Dot(current,desired),-1.0f,1.0f));
        if(angle<0.0001f) return velocity;
        auto axis=cross(current,desired);
        if(Length(axis)<0.0001f)
        {
            axis=cross(current,{0,1,0});
            if(Length(axis)<0.0001f) axis=cross(current,{1,0,0});
        }
        axis=Unit(axis);
        const float step=(std::min)(angle,0.16f),cosine=std::cos(step),sine=std::sin(step);
        const auto perpendicular=cross(axis,current);
        const float parallel=Dot(axis,current)*(1-cosine);
        const auto turn=Unit({current.x*cosine+perpendicular.x*sine+axis.x*parallel,
            current.y*cosine+perpendicular.y*sine+axis.y*parallel,
            current.z*cosine+perpendicular.z*sine+axis.z*parallel});
        return {turn.x*speed,turn.y*speed,turn.z*speed};
    }
    inline void Configure(unsigned char* data,int variant)
    {
        auto word=[data](unsigned offset,unsigned value){std::memcpy(data+offset,&value,4);};
        auto scalar=[data](unsigned offset,float value){std::memcpy(data+offset,&value,4);};
        unsigned flags=0;std::memcpy(&flags,data,4);
        word(0,flags|4);word(4,Object);word(0x110,0x69);word(0x114,0x3C);
        word(0x10,Attack);word(0x14,variant==Stun?6:variant==Explosive?12:20);
        word(0x18,0);word(0x1C,20);scalar(0x160,0.38f);scalar(0x164,250.0f);
    }
    inline int Cycle(int value, int direction) { return (value + direction + Count) % Count; }
    inline int ImpactDamage(int variant) { return variant == Stun ? 6 : variant == Explosive ? 12 : 20; }
    inline int BurnDamage(int maxHp) { return (std::min)(3, (std::max)(1,maxHp / 100)); }
    inline bool CanSelect(unsigned state, bool alive, bool airborne, bool blocked, bool bladeMode)
    { return alive && !airborne && !blocked && !bladeMode && (state <= 4 || (state >= 0x100000 && state <= 0x100002)); }
    inline bool CanSuspend(unsigned state, bool alive, bool blocked, bool bladeMode)
    { return alive && !blocked && !bladeMode && state >= 0x100000 && state <= 0x100002; }
    inline bool CanResume(unsigned state, bool alive, bool aiming, unsigned elapsed)
    { return alive && !aiming && elapsed >= 3 && state <= 4; }
    inline bool Matches(unsigned attack, int damage, int variant, bool owned)
    { return owned && variant > Native && variant < VectorSmg && attack == Attack && damage == ImpactDamage(variant); }
    inline bool TimedDetonation(int variant, unsigned age, bool vanished)
    { return variant == Explosive && age >= 3 && (vanished || age >= 180); }
}
