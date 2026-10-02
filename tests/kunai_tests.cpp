#include "../KunaiPolicy.h"
#include "../EncounterPolicy.h"
#include "../SamStagePolicy.h"
#include <array>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>
int main()
{
    using namespace KunaiPolicy;
    assert(Cycle(Native,-1)==VectorSmg && Cycle(VectorSmg,1)==Native);
    for(unsigned i=0;i<AimHold;++i) assert(!PrecisionAim(i));
    for(unsigned i=AimHold;i<=VolleyCharge;++i) assert(PrecisionAim(i));
    for (unsigned flags=0;flags<16;++flags)
        assert(EncounterPolicy::Scripted(flags&1,flags&2,flags&4,flags&8)==(flags!=0));
    for (unsigned state : {0u,4u,0x100000u,0x100002u})
    {
        assert(EncounterPolicy::CanThrow(state,true,false,false,false));
        assert(!EncounterPolicy::CanThrow(state,true,false,true,false));
        assert(!EncounterPolicy::CanThrow(state,false,false,false,false));
        assert(EncounterPolicy::CanThrow(state,true,true,false,false));
        assert(EncounterPolicy::CanThrow(state,true,false,false,true));
    }
    for (unsigned state : {0x100040u,0x100066u,0x8100u})
        assert(!EncounterPolicy::CanThrow(state,true,false,false,false));
    for(unsigned state:{9u,0xBu,0x50u,0x100005u,0x10000fu,0x10001au,0x10007cu})
    {
        assert(EncounterPolicy::CanThrow(state,true,true,false,false));
        assert(!EncounterPolicy::CanThrow(state,true,true,true,false));
    }
    Charge charge;
    assert(!charge.Tick(true,true,true) && charge.frames==1);
    assert(!charge.Tick(true,true,false) && charge.frames==1); // hit-stop freezes charge
    assert(charge.Tick(false,true,true));
    charge.Cancel(false);
    for(unsigned i=0;i<VolleyCharge-1;++i) assert(!charge.Tick(true,true,true));
    assert(charge.Tick(true,true,true) && charge.frames==VolleyCharge && charge.suppressed);
    for(unsigned i=0;i<120;++i) assert(!charge.Tick(true,true,true)); // one fan per hold
    assert(!charge.Tick(false,true,true) && !charge.suppressed); // no extra release knife
    charge.Cancel(false);
    charge.Tick(true,true,true);
    assert(!charge.Tick(true,false,true)); // scripted event interrupts
    assert(!charge.Tick(true,true,true) && !charge.holding);
    assert(!charge.Tick(false,true,true)); // release after interruption cannot fire
    assert(!charge.Tick(true,true,true) && charge.holding);
    charge.Cancel(false);
    for(unsigned ammo=0;ammo<=10;++ammo) for(unsigned capacity=0;capacity<=MaxShots;++capacity)
        for(unsigned frames:{0u,StunCharge-1,StunCharge,ExplosiveCharge-1,ExplosiveCharge,VolleyCharge-1,VolleyCharge})
        {
            const auto p=Plan(frames,Heatblades,ammo,capacity);
            assert(p.count<=ammo && p.count<=capacity && p.count<=(frames>=VolleyCharge?10u:1u));
            assert(p.homing==(frames>=StunCharge));
            assert(p.variant==(frames>=VolleyCharge ? Heatblades : frames>=ExplosiveCharge ? Explosive : frames>=StunCharge ? Stun : Heatblades));
        }
    assert(Plan(VolleyCharge,Stun,3,32).variant==Stun); // volley retains selected tap payload
    assert(Plan(VolleyCharge,Heatblades,10,32).count==10);
    for(unsigned count=1;count<=VolleySize;++count)
        for(unsigned i=0;i<count;++i)
        {
            const float angle=FanSpread(i,count);
            assert(std::abs(angle)<=FanHalfAngle+0.00001f);
            assert(std::abs(angle+FanSpread(count-1-i,count))<0.00001f);
            if(i) assert(angle>FanSpread(i-1,count));
        }
    assert(FanSpread(0,10)==-FanHalfAngle && FanSpread(9,10)==FanHalfAngle);
    Burst burst;
    unsigned elapsed=0;
    for(unsigned i=0;i<BurstSize;++i)
    {
        assert(burst.Capacity()==BurstSize-i);
        burst.Fired(1,i==BurstSize-1);
        assert(burst.used==i+1 && !burst.Capacity());
        if(i<BurstSize-1)
            for(unsigned frame=0;frame<RapidInterval;++frame) {burst.Tick(true);++elapsed;}
    }
    assert(elapsed==54 && burst.recovering && burst.remaining==BurstCooldown);
    burst.Tick(false);assert(burst.remaining==BurstCooldown); // cutscene/hit-stop freeze
    for(unsigned frame=0;frame<BurstCooldown-1;++frame) burst.Tick(true);
    assert(!burst.Capacity() && burst.used==10);
    burst.Tick(true);assert(burst.Capacity()==10 && !burst.recovering && !burst.used);
    burst.Fired(0,false);assert(burst.Capacity()==10); // failed spawn never advances burst
    burst.Fired(3,false);assert(burst.used==3 && burst.remaining==RapidInterval);
    for(unsigned frame=0;frame<RapidInterval;++frame) burst.Tick(true);
    burst.Fired(3,false);
    for(unsigned frame=0;frame<RapidInterval;++frame) burst.Tick(true);
    burst.Fired(3,false);
    for(unsigned frame=0;frame<RapidInterval;++frame) burst.Tick(true);
    assert(Plan(VolleyCharge,Heatblades,10,burst.Capacity()).count==1);
    burst.Fired(1,false);assert(burst.recovering); // replenished ammo cannot bypass burst limit
    burst={};burst.Fired(2,true);assert(burst.recovering); // partially filled inventory exhausted
    for(unsigned state:{0u,4u,0x100000u,0x100002u})
    {
        assert(CanAnimateThrow(state,false,false));
        assert(!CanAnimateThrow(state,true,false));
        assert(!CanAnimateThrow(state,false,true));
    }
    for(unsigned state:{9u,0x100005u,0x100040u,0x10007cu}) assert(!CanAnimateThrow(state,false,false));
    std::ifstream throwFile("local_assets/data000/pl/pl0010.dat.unpacked/pl0010_2561.mot",std::ios::binary);
    assert(throwFile);
    std::array<char,12> throwHeader{};throwFile.read(throwHeader.data(),throwHeader.size());
    assert(throwFile && SamStagePolicy::Length(throwHeader.data())==131);
    std::ifstream releaseFile("local_assets/data000/pl/pl0010.dat.unpacked/pl0010_2566.mot",std::ios::binary);
    std::array<char,12> releaseHeader{};releaseFile.read(releaseHeader.data(),releaseHeader.size());
    assert(releaseFile && SamStagePolicy::Length(releaseHeader.data())==86);
    assert(PoseTicks(131,true)==53 && PoseTicks(86,false)==29);
    assert(CanStartPose(false,false,true) && CanStartPose(false,false,false));
    assert(CanStartPose(true,true,false)); // held aim transitions directly to throw
    assert(!CanStartPose(true,false,true)); // held input cannot overwrite the throw
    assert(!CanStartPose(true,false,false)); // rapid shots cannot restart the throw
    assert(!CanStartPose(true,true,true)); // aim stays settled while held
    ThrowPose pose;pose.Begin(34,7,5);
    for(unsigned i=0;i<6;++i) assert(!pose.Tick(true,true,true));
    pose.Begin(34,8,4);assert(pose.remaining==28 && pose.node==7 && pose.returnMap==5);
    assert(!pose.Tick(false,true,true) && pose.remaining==28);
    for(unsigned i=0;i<27;++i) assert(!pose.Tick(true,true,true));
    assert(pose.Tick(true,true,true) && !pose.remaining && pose.returnMap==5);
    pose.Begin(34,9,4);assert(!pose.Tick(true,false,true) && !pose.remaining);
    pose.Begin(34,10,4);assert(!pose.Tick(true,true,false) && !pose.remaining);
    pose.Begin(34,-1,4);assert(!pose.remaining); // failed native playback needs no recovery
    assert(AimScore({0,0,10},{0,0,1},60,0.995f)>=0);
    assert(AimScore({0,0,-10},{0,0,1},60,0.995f)<0);
    assert(AimScore({10,0,10},{0,0,1},60,0.995f)<0); // precision aim excludes off-reticle targets
    assert(AimScore({0,0,61},{0,0,1},60,0.995f)<0);
    assert(ReticleX==0.5f && ReticleY<0.5f && ReticleY>0.4f);
    // A camera behind/above the muzzle must converge on the reticle at the
    // target's depth, without redirecting laterally to a nearby enemy's chest.
    for(float depth:{5.0f,12.0f,40.0f,60.0f})
    {
        const Vector camera{2,3,-4},ray=Unit({0,0.12f,1}),muzzle{0,1.1f,0.65f};
        const Vector target{camera.x+ray.x*depth,camera.y+ray.y*depth,camera.z+ray.z*depth};
        const Vector offset{target.x-camera.x,target.y-camera.y,target.z-camera.z};
        assert(std::abs(AimDepth(offset,ray)-depth)<0.0001f);
        const Vector velocity=Unit({target.x-muzzle.x,target.y-muzzle.y,target.z-muzzle.z});
        const float travel=Length({target.x-muzzle.x,target.y-muzzle.y,target.z-muzzle.z});
        const Vector reached{muzzle.x+velocity.x*travel,muzzle.y+velocity.y*travel,muzzle.z+velocity.z*travel};
        assert(Length({reached.x-target.x,reached.y-target.y,reached.z-target.z})<0.0001f);
    }
    assert(AimDepth({0,0,-5},{0,0,1})==1 && AimDepth({0,0,100},{0,0,1})==60);
    Vector velocity{0,0,0.38f};
    const Vector movingTarget{4,3,10};
    const float before=Dot(Unit(velocity),Unit(movingTarget));
    for(unsigned i=0;i<60;++i)
    {
        const auto previous=velocity;
        velocity=Steer(velocity,movingTarget);
        assert(std::isfinite(Length(velocity)) && std::abs(Length(velocity)-0.38f)<0.00001f);
        assert(Dot(Unit(previous),Unit(velocity))>=std::cos(0.161f));
    }
    assert(Dot(Unit(velocity),Unit(movingTarget))>before);
    assert(Length(Steer({0,0,0},{1,2,3}))==0); // native stopped projectile remains stopped
    assert(std::isfinite(Length(Steer({0,0,0.38f},{0,0,-10}))));
    velocity={0,0,0.38f};
    for(unsigned i=0;i<30;++i) velocity=Steer(velocity,{0,0,-10});
    assert(Dot(Unit(velocity),Vector{0,0,-1})>0.99f); // target passing behind still turns smoothly
    for(unsigned state:{0u,1u,4u,0x100000u,0x100002u})
    {
        assert(CanSelect(state,true,false,false,false));
        assert(!CanSelect(state,true,false,true,false));
        assert(!CanSelect(state,true,true,false,false));
        assert(!CanSelect(state,true,false,false,true));
        assert(!CanSelect(state,false,false,false,false));
    }
    for(unsigned state:{9u,0x50u,0x100012u,0x100040u,0x500u}) assert(!CanSelect(state,true,false,false,false));
    assert(CanSuspend(0x100000,true,false,false));
    assert(!CanSuspend(0x100012,true,false,false));
    assert(!CanSuspend(0x100000,true,true,false));
    assert(!CanResume(0,true,true,90)); // never restore Sam while aim is held
    assert(!CanResume(0,true,false,2)); // give native graph time to enter aim
    assert(!CanResume(0x50,true,false,30)); // grenade recovery must complete
    assert(CanResume(0,true,false,30));
    for(int variant=Stun;variant<VectorSmg;++variant)
    {
        std::array<unsigned char,0x320> original{},shot{};
        for(unsigned i=0;i<original.size();++i) original[i]=static_cast<unsigned char>(i*7+3);
        shot=original;Configure(shot.data(),variant);
        unsigned object=0;std::memcpy(&object,shot.data()+4,4);assert(object==Object);
        // Preserve friendly collision flags, owner pointer/handle, all native
        // aiming vectors, target attachment and grenade inventory metadata.
        for(unsigned i=0;i<shot.size();++i)
        {
            const bool changed=i<8 || (i>=0x10 && i<0x20) || (i>=0x110 && i<0x118) || (i>=0x160 && i<0x168);
            if(!changed) assert(shot[i]==original[i]);
        }
        assert(Matches(Attack,ImpactDamage(variant),variant,true));
        assert(!Matches(Attack,ImpactDamage(variant),variant,false));
        assert(!Matches(0x57,ImpactDamage(variant),variant,true));
        assert(ImpactDamage(variant)<=20);
        assert(!TimedDetonation(variant,1,true));
    }
    assert(TimedDetonation(Explosive,3,true));
    assert(TimedDetonation(Explosive,180,false));
    assert(!TimedDetonation(Stun,180,true));assert(!TimedDetonation(Heat,180,true));
    assert(BurnDamage(50)==1 && BurnDamage(100000)==3);
    for(const char* name:{"wp0372.wmb","wp0372.eff","wp0372_col.hkx"})
        assert(std::filesystem::file_size(std::string("local_assets/data000/wp/wp0372.dat.unpacked/")+name)>0);
    std::cout<<"PASS: rapid ten-knife burst/recovery, native throw motion, charge cancellation, ammo/volley caps, air/combo eligibility, precision aim and bounded homing; native collision ownership preserved.\n";
}
