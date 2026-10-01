#include "../SamTogglePolicy.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace SamTogglePolicy;
    for (unsigned action : {0u, 3u, 9u, 0xBu, 0x4Fu, 0x51u, 0x5Bu, 0x5Du})
    {
        assert(CanActivate(action, true));
        assert(!CanActivate(action, false));
    }
    for (unsigned action : {0x47u, 0x134u, 0x201u, 0xFFFFFFFFu})
        assert(!CanActivate(action, true));
    assert(EntryState(true) == 0x100005u);
    assert(ExitState(true) == 0xBu);
    assert(EntryState(false) == 0x100000u);
    assert(ExitState(false) == 0u);
    for (unsigned action : {0x100004u, 0x100010u, 0x100011u, 0x100017u, 0x100019u, 0x10001Au})
        assert(OwnsAction(action));
    assert(!OwnsAction(0x47));
    assert(!OwnsAction(0x201));
    std::cout << "PASS: airborne transitions, mid-combat exits, scripted-state and death activation guards\n";
}
