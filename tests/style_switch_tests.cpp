#include "../1-StyleSwitch.h"
#include <cassert>
#include <iostream>

int main()
{
    // Exercise the production class with a map
    std::vector<AnimationMap::Unit> entries(1400);
    for (int i = 0; i < 1400; ++i) { entries[i].m_Id = i; entries[i].cancel = 87; }
    memcpy(entries[0].m_pName, "2000", 4);
    memcpy(entries[1].m_pName, "2001", 4);
    memcpy(entries[2].m_pName, "2002", 4);
    memcpy(entries[1399].m_pName, "2029", 4);
    lib::AllocatedArray<AnimationMap::Unit> units{entries.data(), 1400, 1400};
    AnimationMap original{&units};
    Entity entity;
    Pl0000 player{&entity, &original};
    entity.m_pBehavior = &player;

    StyleSwitch style;
    style.SetStyle(&player, true);

    // Map pointer remains unchanged (in-place patching avoids dangling pointers)
    assert(player.m_pAnimationMap == &original);
    // Swapped units now match Sam animation codes
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[0].m_pName, "9000", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[1].m_pName, "9001", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[2].m_pName, "9002", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[1399].m_pName, "9108", 4) == 0);
    assert(player.m_pAnimationMap->m_pUnits->m_pArray[0].cancel == 87);
    assert(player.m_pAnimationMap->m_pUnits->m_pArray[1399].m_Id == 1399);

    // Repeated ticks do not corrupt mapping
    style.SetStyle(&player, true);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[0].m_pName, "9000", 4) == 0);

    // Toggle off restores original names exactly
    style.SetStyle(&player, false);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[0].m_pName, "2000", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[1].m_pName, "2001", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[2].m_pName, "2002", 4) == 0);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[1399].m_pName, "2029", 4) == 0);

    // Re-enable and reset
    style.SetStyle(&player, true);
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[0].m_pName, "9000", 4) == 0);
    style.Reset();
    assert(memcmp(player.m_pAnimationMap->m_pUnits->m_pArray[0].m_pName, "2000", 4) == 0);

    std::cout << "PASS: in-place unit swapping, metadata preservation, toggle restoration, repeated ticks, reset\n";
    return 0;
}
