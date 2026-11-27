#include "../SaiMovesetPolicy.h"
#include "../SaiPullPolicy.h"
#include "../MonsoonKickPolicy.h"
#include "../SamAttackInputPolicy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace SaiMovesetPolicy;
    // Regress the real source observed in the runtime log: Heavy attacks stay
    // in 9C. Menu IDs and native recovery states are not new move handlers.
    assert(StabbingDespair==0x9C && ForceOfMagnetism==0x9D);
    assert(Find(DoubleThrowStab)->menu==36 && Find(SpinSlice)->menu==38);
    assert(Find(Sommersault)->menu==39 && Find(UnavoidableFate)->menu==40);
    // Runtime entered 161 but played the A2 forward-flip route. Verify the
    // spin dispatch and startup motion as a pair, separately from Sommersault.
    assert(Find(SpinSlice)->native==0xA1 && Find(SpinSlice)->motion==10);
    assert(Find(Sommersault)->native==0xA2 && Find(Sommersault)->motion==22);
    assert(Motion(SpinSlice,10)==10); // pl0010_2b20, not 2b70
    assert(!Find(0xA0) && !Find(0xA4) && !Find(0xA5));
    // First two presses stay native: no custom result and no reserved buffer.
    HeavyChain chainGate;
    Buffer gated;
    auto press=[&](unsigned state,bool heavy,bool light,float frame) {
        if(!chainGate.Poll(state,heavy,light)){gated.Reset();return 0u;}
        return gated.Poll(state,false,heavy,light,true,frame);
    };
    assert(!press(0,true,false,0) && chainGate.presses==1 && !gated.ticks);
    assert(!press(0x9C,false,false,25) && chainGate.presses==1);
    assert(!press(0x9C,true,false,25) && chainGate.presses==2 && !gated.ticks);
    assert(!press(0x9D,false,false,25) && chainGate.presses==2);
    assert(press(0x9D,true,false,25)==SpinSlice); // third fresh press
    assert(press(SpinSlice,true,false,40)==Sommersault); // fourth
    assert(!press(0,false,false,0) && !chainGate.presses); // string ended
    assert(!press(0,true,false,0) && chainGate.presses==1);
    assert(!press(0x9C,true,false,25) && chainGate.presses==2);
    assert(!press(0x9C,true,false,2) && gated.ticks); // third buffers through startup
    assert(press(0x9C,false,false,20)==DoubleThrowStab);
    assert(!press(0x46,true,false,30) && !chainGate.presses); // damage
    assert(!press(0x9C,true,false,30) && chainGate.presses==1);
    assert(!press(0x9C,true,true,30) && !chainGate.presses); // Light/chord
    AirChain aerial;
    Buffer airBuffer;
    auto airPress=[&](unsigned state,bool heavy,bool initialized,float frame,bool frozen=false) {
        if(!aerial.Observe(state,true,initialized,false)){airBuffer.Reset();return 0u;}
        const unsigned next=airBuffer.Poll(state,true,heavy,false,initialized,frame,frozen);
        if(next==UnavoidableFate)aerial.Consume();
        return next;
    };
    assert(!airPress(0xA,true,true,0) && !airBuffer.ticks); // first air Heavy stays native
    assert(!airPress(NativeAirHeavy,false,false,0)); // must actually initialize
    assert(!airPress(NativeAirHeavy,false,true,10)); // observing/holding is not another tap
    assert(!airPress(NativeAirHeavy,true,true,10)); // second fresh air Heavy buffers
    assert(airBuffer.ticks);
    assert(!airPress(NativeAirHeavy,false,true,10,true));
    assert(airPress(NativeAirHeavy,false,true,20)==UnavoidableFate);
    assert(!airPress(UnavoidableFate,true,true,40)); // no repeated added strikes
    assert(!airPress(0xB,true,true,0)); // spent until landing
    assert(!aerial.Observe(0,false,true,false)); // landing resets
    assert(!airPress(0xB,true,true,0)); // new jump still requires regular air Heavy
    assert(!chainGate.Poll(0xB,true,false) && !chainGate.presses); // air cannot arm ground chain
    assert(!airPress(NativeAirHeavy,false,true,10));
    assert(!airPress(NativeAirHeavy,true,true,10));
    assert(airPress(0xB,false,true,0)==UnavoidableFate); // buffer survives native recovery
    aerial.Reset();airBuffer.Reset();
    assert(!airPress(NativeAirHeavy,false,true,25));
    assert(!aerial.Observe(NativeAirHeavy,true,true,true)); // Light cancels
    assert(!airPress(0xB,true,true,0));
    assert(!aerial.Observe(0x46,true,true,false)); // damage cancels
    chainGate.Reset();assert(!chainGate.Poll(0x9C,true,false)); // pull/kick reset
    Buffer b;
    assert(!b.Poll(0x9C,false,true,false,true,17));
    assert(b.Poll(0x9C,false,false,false,true,20)==DoubleThrowStab);
    assert(!b.Poll(DoubleThrowStab,false,true,false,true,30));
    assert(!b.Poll(DoubleThrowStab,false,false,false,true,36));
    assert(b.Poll(DoubleThrowStab,false,false,false,true,48)==SpinSlice);
    assert(!b.Poll(SpinSlice,true,true,false,true,30));
    assert(b.Poll(SpinSlice,true,false,false,true,36)==Sommersault);
    assert(!b.Poll(Sommersault,false,true,false,true,90));
    assert(b.Poll(0x9D,false,true,false,true,20)==SpinSlice);
    assert(b.Poll(ThrowRecovery,false,true,false,true,8)==DoubleThrowStab);
    assert(b.Poll(ForceRecovery,false,true,false,true,8)==SpinSlice);
    for(unsigned state:{9u,0xAu,0xBu})
        assert(b.Poll(state,true,true,false,true,0)==UnavoidableFate);
    assert(!b.Poll(UnavoidableFate,true,true,false,true,40));
    assert(!b.Poll(0x46,true,true,false,true,80));
    assert(!b.Poll(0x9C,false,true,true,true,80));
    assert(!b.Poll(0x9C,false,true,false,false,0));
    for(unsigned i=0;i<BufferTicks+21;++i)assert(!b.Poll(0x9C,false,false,false,false,0));
    assert(!b.Poll(0x9C,false,false,false,true,30));
    assert(!b.Poll(SpinSlice,true,true,false,true,35,true));
    for(unsigned i=0;i<100;++i)assert(!b.Poll(SpinSlice,true,false,false,true,35,true));
    assert(b.Poll(SpinSlice,true,false,false,true,36)==Sommersault);
    // Each move owns its identity across native startup/attack/recovery phases.
    Machine machine;
    for(const auto& move:Moves) {
        assert(machine.Enter(move.action));
        assert(machine.native==move.native);
        assert(machine.Complete(move.native)==move.action);
        assert(machine.Complete(0xA3)==move.action);
        assert(machine.native==0xA3);
        assert(machine.Complete(0)==0 && machine.action==0);
        assert(machine.Enter(move.action));
        assert(machine.Complete(0x46)==0x46 && machine.action==0); // damage interrupts
    }
    assert(!machine.Enter(36)); // never dispatch a menu ID
    assert(machine.Enter(SpinSlice));
    assert(!machine.Finished(0,0)); // startup cannot be mistaken for completion
    assert(!machine.Finished(1,0)); // intermediate attack clips can end
    assert(machine.Complete(0x9F)==SpinSlice);
    assert(!machine.Finished(0,0)); // native recovery has not initialized yet
    assert(!machine.Finished(1,2));
    assert(!machine.Finished(1,0.5f));
    assert(machine.Finished(1,0)); // final recovery ends without another input
    assert(machine.Enter(Sommersault));
    assert(!machine.Finished(1,0));
    assert(machine.Recovering(7)); // permits grounded Back+Y before natural exit
    assert(!machine.Finished(7,1));
    assert(machine.Finished(7,0)); // A2 terminal phase must not remain stuck
    assert(machine.Enter(UnavoidableFate));
    assert(!machine.Recovering(1));
    assert(!machine.Finished(3,0)); // falling loop waits for an actual landing
    assert(AerialLift::Landed(NativeAirHeavy,3,false));
    assert(!AerialLift::Landed(NativeAirHeavy,3,true));
    assert(!AerialLift::Landed(NativeAirHeavy,1,false)); // startup is preserved
    assert(!AerialLift::Landed(0xA2,3,false)); // other moves unaffected
    assert(machine.Recovering(5));
    assert(!machine.Finished(5,0.5f));
    assert(machine.Finished(5,0)); // landing recovery reliably completes
    machine.ticks=AerialLift::MaxAttackTicks-1;
    assert(!machine.Finished(3,20));
    ++machine.ticks;
    assert(machine.Finished(3,20)); // stalled airborne attack has a bounded exit
    assert(machine.Complete(0xB)==0xB && !machine.action); // return to ordinary free-fall
    AerialLift lift;
    lift.Start(1.0f);
    assert(lift.Velocity(-0.3f,0,0,1.0f)>=AerialLift::LaunchSpeed);
    assert(lift.Velocity(0.21f,1,1,1.1f)<=AerialLift::MaxSpeed);
    assert(lift.Velocity(-0.3f,AerialLift::SupportTicks,1,1.0f)==-0.3f);
    assert(lift.Velocity(-0.3f,5,2,1.0f)==-0.3f); // recovery restores normal gravity
    assert(lift.Velocity(-0.3f,1,1,1.0f+AerialLift::MaxRise)==-0.3f);
    float altitude=1.0f,velocity=-0.3f;
    for(unsigned tick=0;tick<AerialLift::SupportTicks;++tick) {
        velocity=lift.Velocity(velocity,tick,1,altitude);
        altitude+=velocity;
        velocity-=0.03f;
        assert(altitude>0); // descending near-ground start survives the strike
    }
    for(unsigned tick=AerialLift::SupportTicks;tick<120;++tick) {
        const float falling=velocity;
        velocity=lift.Velocity(velocity,tick,1,altitude);
        assert(velocity==falling); // no permanent hover or repeated acceleration
        altitude+=velocity;velocity-=0.03f;
    }
    assert(altitude<0);
    lift.Reset();assert(lift.Velocity(-0.3f,0,0,1.0f)==-0.3f);
    assert(Motion(DoubleThrowStab,0)==5 && Motion(DoubleThrowStab,1)==6 && Motion(DoubleThrowStab,2)==7);
    assert(Motion(SpinSlice,19)==19);
    assert(Motion(Sommersault,19)==22 && Motion(Sommersault,20)==27);
    assert(Motion(UnavoidableFate,14)==14);
    // A fresh Heavy is captured before native selection. A held press and
    // direction modifier bits cannot advance the next state.
    SamAttackInputPolicy::Reader reader;
    assert(reader.Poll(0x100,0,0x20,0x101).heavy);
    assert(!reader.Poll(0x101,0,0x20,0x101).heavy);
    reader.Poll(0,0,0x20,0x101);
    assert(reader.Poll(0x100,0,0x20,0x101).heavy);
    const unsigned chain[]={StabbingDespair,DoubleThrowStab,ForceOfMagnetism,SpinSlice,Sommersault};
    for(unsigned state:chain)assert(SaiPullPolicy::Safe(state,true,false,false,false,true));
    assert(MonsoonKickPolicy::CanStart(Sommersault,true,false,false,false,true,true,false,0,-1,true,36));
    assert(!SaiPullPolicy::Safe(MonsoonKickPolicy::Action,true,false,false,false,true));
    assert(!b.Poll(SpinSlice,true,true,false,false,0));
    b.Reset();assert(!b.Poll(SpinSlice,true,false,false,true,40));
    std::puts("Sai registered state machine, native phases, motion routes and Heavy edge regressions passed.");
}
