#include "../MonsoonKickPolicy.h"
#include <cassert>
#include <cstdio>
int main() {
    using namespace MonsoonKickPolicy;
    assert(CanStart(0,true,false,false,false,true,true,false,0,-1));
    assert(CanStart(4,true,false,false,false,true,true,false,0,-0.8f));
    assert(!CanStart(0,true,false,false,false,true,true,false,0,1));
    assert(!CanStart(0,true,false,false,false,true,true,false,1,-1));
    assert(!CanStart(0,true,false,false,false,true,false,false,0,-1));
    assert(!CanStart(0,true,false,false,false,true,true,true,0,-1));
    assert(!CanStart(0,true,false,false,false,false,true,false,0,-1));
    assert(!CanStart(0,false,false,false,false,true,true,false,0,-1));
    assert(!CanStart(0,true,true,false,false,true,true,false,0,-1));
    assert(!CanStart(0,true,false,true,false,true,true,false,0,-1));
    assert(!CanStart(0,true,false,false,true,true,true,false,0,-1));
    assert(!CanStart(Action,true,false,false,false,true,true,false,0,-1));
    assert(!CanStart(0x46,true,false,false,false,true,true,false,0,-1));
    const unsigned recoveries[]={0x9F,0xA0,0xA3,0xA4,0xA5};
    for(unsigned state:recoveries) {
        assert(CanStart(state,true,false,false,false,true,true,false,0,-1,true,8));
        assert(!CanStart(state,true,false,false,false,true,true,false,0,-1,true,7));
        assert(!CanStart(state,true,false,false,false,true,true,false,0,-1,false,30));
        assert(!CanStart(state,true,true,false,false,true,true,false,0,-1,true,30));
        assert(!CanStart(state,true,false,false,false,true,true,true,0,-1,true,30));
        assert(!CanStart(state,true,false,false,false,true,true,false,0,1,true,30));
        assert(StartState(0x9D,state,true,true)==state);
        assert(StartState(0x46,state,true,true)==0x46); // damage is not a cancel
        assert(StartState(0x9D,state,false,true)==0x9D);
    }
    assert(!CanStart(0xA1,true,false,false,false,true,true,false,0,-1,true,30));
    assert(!CanStart(0xA2,true,false,false,false,true,true,false,0,-1,true,30));
    assert(!CanStart(0x9E,true,false,false,false,true,true,false,0,-1,true,30));
    SommersaultBuffer somersault;
    const unsigned som=SaiMovesetPolicy::Sommersault;
    assert(!somersault.Poll(som,5,true,true,true,false,true)); // early Back+Y while airborne
    assert(!somersault.Poll(som,35,true,false,false,false,false)); // startup protected
    assert(somersault.Poll(som,36,true,false,false,false,false)); // replay on landing
    assert(!somersault.Poll(som,37,true,false,false,false,false)); // only once
    assert(CanStart(som,true,false,false,false,true,true,false,0,-1,true,36));
    assert(!CanStart(som,true,true,false,false,true,true,false,0,-1,true,36));
    assert(!somersault.Poll(som,5,true,true,true,false,true,true));
    for(unsigned n=0;n<100;++n)assert(!somersault.Poll(som,5,true,true,false,false,false,true));
    assert(somersault.Poll(som,36,true,false,false,false,false)); // hit-stop preserves queued kick
    assert(!somersault.Poll(som,5,true,true,true,false,true));
    assert(!somersault.Poll(som,10,true,true,false,true,false)); // Light cancels
    assert(!somersault.Poll(som,36,true,false,false,false,false));
    assert(!somersault.Poll(som,5,true,true,true,false,true));
    assert(!somersault.Poll(0x46,36,true,false,false,false,false)); // damage cancels
    assert(!somersault.Poll(som,36,true,false,false,false,false));
    assert(!CanJumpCancel(53,true));
    assert(CanJumpCancel(54,true));
    assert(!CanJumpCancel(80,false));
    assert(StartState(0x46,0,true)==0x46);
    assert(StartState(0x5B,4,true)==0x5B);
    assert(StartState(0x46,0,false)==0x46);
    assert(StartState(0x46,0x46,true)==0x46);
    assert(StartState(0xC6,0,true)==0xC6);
    assert(!WeaponAllowed(0,0,false));
    assert(WeaponAllowed(2,3,false));
    assert(!WeaponAllowed(2,0,false));
    assert(!WeaponAllowed(0,3,false));
    assert(!WeaponAllowed(2,3,true));
    assert(!WeaponAllowed(1,2,false));
    assert(StartState(0x9D,0,true,true)==0);
    assert(StartState(0x9C,3,true,true)==3);
    assert(StartState(0x9C,3,true,false)==0x9C);
    assert(StartState(0x9C,3,false,true)==0x9C);
    assert(StartState(0x9C,0x9C,true,true)==0x9C);
    assert(StartState(0xA1,4,true,true)==4);
    assert(StartState(0xA1,0,true,false)==0xA1);
    assert(StartState(0xA1,0,false,true)==0xA1);
    assert(ConfirmedLaunch(100,80,true,44,false));
    assert(!ConfirmedLaunch(100,100,true,44,false));
    assert(!ConfirmedLaunch(100,80,false,44,false));
    assert(!ConfirmedLaunch(100,80,true,30,false));
    assert(!ConfirmedLaunch(100,80,true,44,true));
    assert(!ConfirmedLaunch(100,0,true,44,false));
    assert(ConfirmedLaunch(100,80,true,63,false));
    assert(!ConfirmedLaunch(100,80,true,65,false));
    assert(Command(true,false,false,false)==FollowUp::Light);
    assert(Command(false,true,false,false)==FollowUp::Heavy);
    assert(Command(true,true,false,false)==FollowUp::Chord);
    assert(Command(false,false,true,false)==FollowUp::Jump);
    assert(Command(false,false,false,true)==FollowUp::Pull);
    CancelBuffer buffer;
    assert(buffer.Poll(40,true,FollowUp::Light)==FollowUp::None);
    assert(buffer.Poll(44,true,FollowUp::None)==FollowUp::Light);
    assert(buffer.Poll(45,true,FollowUp::None)==FollowUp::None); // single replay
    assert(buffer.Poll(54,false,FollowUp::Heavy)==FollowUp::None);
    assert(buffer.Poll(63,false,FollowUp::None)==FollowUp::None);
    assert(buffer.Poll(64,false,FollowUp::None)==FollowUp::Heavy);
    assert(buffer.Poll(44,true,FollowUp::Jump)==FollowUp::Jump);
    assert(buffer.Poll(44,true,FollowUp::Pull)==FollowUp::Pull);
    assert(buffer.Poll(20,false,FollowUp::Light)==FollowUp::None);
    for(unsigned n=0;n<12;++n)assert(buffer.Poll(21,false,FollowUp::None)==FollowUp::None);
    assert(buffer.Poll(64,false,FollowUp::None)==FollowUp::None); // expired edge
    assert(buffer.Poll(43,true,FollowUp::Heavy,true)==FollowUp::None);
    for(unsigned n=0;n<30;++n)assert(buffer.Poll(43,true,FollowUp::None,true)==FollowUp::None);
    assert(buffer.Poll(44,true,FollowUp::None)==FollowUp::Heavy); // hit-stop preserves buffer
    assert(buffer.Poll(40,true,FollowUp::Light)==FollowUp::None);
    buffer.Clear();assert(buffer.Poll(64,true,FollowUp::None)==FollowUp::None);
    std::puts("Monsoon kick input and base-moveset isolation tests passed.");
}
