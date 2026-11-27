#include "../SaiPullPolicy.h"
#include <cassert>
#include <cstdio>
int main() {
    using namespace SaiPullPolicy;
    assert(Press(false, false, LeftStickButton, 0, 0));
    assert(Press(false, false, 0, LeftStickButton, 0));
    assert(!Press(false, false, LeftStickButton, 0, LeftStickButton));
    assert(Press(false, false, LeftStickButton | 0x8000, 0, 0));
    assert(!Press(false, false, 0x8000, 0, 0));
    assert(Press(true, false, 0, 0, 0));
    assert(!Press(true, true, 0, 0, 0));
    assert(Safe(0, true, false, false, false));
    assert(!Safe(0xA1, true, false, false, false));
    const unsigned pullStates[] = {0, 0x9D, 0xA1, 0x9F, 0xA0, 0xA2, 0xA3, 0xA4, 0xA5};
    for (unsigned state : pullStates) {
        assert(Safe(state, true, false, false, false, true));
        assert(!Safe(state, false, false, false, false, true));
        assert(Safe(state,true,true,false,false,true)==SaiAttack(state));
        assert(!Safe(state, true, false, true, false, true));
        assert(!Safe(state, true, false, false, true, true));
    }
    assert(!Safe(0x46, true, false, false, false, true));
    assert(Safe(0xA2, true, false, false, false, true));
    assert(!Safe(0x9E, true, false, false, false, true));
    assert(!Safe(9,true,true,false,false,true));
    assert(Eligible(0x20140));
    assert(!Eligible(0x20170));
    assert(!Eligible(0x20310));
    assert(Step(StopDistance) == 0);
    assert(Step(Range) == Speed);
    float distance=Range;
    for(unsigned tick=0;tick<Duration;++tick) {
        const float step=Step(distance);
        assert(step>=0 && step<=Speed);
        distance-=step;
        assert(distance>=StopDistance);
    }
    assert(std::fabs(distance-StopDistance)<0.001f);
    assert(SaiAttack(0xA1) && SaiAttack(0x9F));
    assert(SaiAttack(0xA2) && !SaiAttack(0x46));
    std::puts("Sai pull input and attack lifecycle tests passed.");
}
