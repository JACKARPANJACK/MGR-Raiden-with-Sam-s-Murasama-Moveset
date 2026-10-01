#include "../SamDirectionalPolicy.h"
#include "../SamUltimatePolicy.h"
#include "../SamRoundTripPolicy.h"
#include "../SamEffectPolicy.h"
#include <cassert>
#include <set>
#include <iostream>
int main()
{
    using namespace SamDirectionalPolicy;
    Input input;
    assert(input.Poll(0, 0, true, false) == -1);
    assert(input.Poll(0, 1, true, false) == 5);
    input.Reset();
    input.Poll(0, 1, false, false);
    input.Poll(0, 0, false, false);
    assert(input.Poll(0, 0, false, true) == 7); // flick survives stick release
    assert(input.Poll(0, 0, false, true) == -1); // consumed, no retrigger
    input.Reset();
    input.Poll(0, 1, false, false);
    for (int i = 0; i < 8; ++i) input.Poll(0, 1, false, false);
    assert(input.Poll(0, 1, false, true) == 1); // held direction charge slash
    input.Reset();
    assert(input.Poll(0, -1, false, true) == 12);
    assert(input.Poll(1, 0, true, true) == -1); // native dodge stays available
    assert(Resolve(0.1f, 0.2f) == None);
    assert(Resolve(-1, 0.7f) == Left);
    std::set<int> addons;
    for (auto dir : {Forward, Back, Left, Right})
        for (bool heavy : {false, true}) for (bool flick : {false, true})
            addons.insert(Move(dir, heavy, flick));
    assert(addons.size() == 11);
    for (unsigned index : SamUltimatePolicy::UltimateIndices)
        assert(!addons.count(static_cast<int>(index)));
    assert(SamUltimatePolicy::UltimateIndex(3) == 8);
    using namespace SamRoundTripPolicy;
    assert(!CanHit(0, 0, 10, true));
    assert(!CanHit(0, 0, 1, false));
    assert(CanHit(0, 0, 2.5f, true));
    float x=0, y=0, z=0;
    Step(x, y, z, 0, 0, 0.2f, 0.7f);
    assert(std::fabs(z-0.2f) < 0.001f); // no return overshoot
    Step(x, y, z, 0, 0, 0.2f, 0.7f);
    assert(std::isfinite(z));
    using SamEffectPolicy::Remap;
    assert(Remap(333, 0x7C7980, true) == 117);
    assert(Remap(334, 0x7C7A30, true) == 118);
    assert(Remap(335, 0x7C7CE0, true) == 119);
    assert(Remap(181, 0x7F71A0, true) == 121);
    assert(Remap(242, 0x7B9BE0, true) == 242); // preserve shared bank effects
    assert(Remap(333, 0x7B9BE0, true) == 333); // native Sam ID unchanged
    assert(Remap(333, 0x7C7980, false) == 333); // other actors / G off
    assert(Remap(333, 0x400000, true) == 333); // unrelated effect caller
    std::cout << "PASS: directional/flick inputs, all boss add-ons, ultimate separation, proximity damage, trajectory, scoped Sam effects\n";
}
