#include "../SamElectricPolicy.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace SamElectricPolicy;
    assert(Ordinary(0x20140)); assert(!Ordinary(0x20020)); assert(!Ordinary(0x201A0));
    assert(!Ordinary(0x20141)); // detached body parts cannot be held/stunned
    assert(ConfirmedHit(100,90,true,true,2));
    assert(!ConfirmedHit(-1,90,true,true,2)); // first observation
    assert(!ConfirmedHit(100,100,true,true,2)); // whiff/block
    assert(!ConfirmedHit(100,90,true,false,2)); // another actor's hit
    assert(!ConfirmedHit(100,90,false,true,2)); // scripted damage
    assert(!ConfirmedHit(100,0,true,true,2)); // dead target
    assert(!ConfirmedHit(100,90,true,true,6));
    unsigned light=0,heavy=0;
    for (unsigned roll=0;roll<100;++roll)
    {light+=Proc(roll,false,0); heavy+=Proc(roll,true,0); assert(!Proc(roll,true,1));}
    assert(light==12 && heavy==25);
    assert(Finisher(25,100,3)); assert(!Finisher(26,100,3));
    assert(!Finisher(0,100,2)); assert(!Finisher(25,100,3.1f));
    std::cout << "PASS: owned-hit EMP eligibility, exact proc chances, cooldown, boss/parts immunity, finishing range and HP gates\n";
}
