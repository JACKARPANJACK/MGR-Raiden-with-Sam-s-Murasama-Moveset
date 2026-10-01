#pragma once
namespace EncounterPolicy
{
    // Story events and QTEs run Raiden's native graph. Object stop also covers
    // harmless pause/hit-stop; it must not tear down Sam's combat controller.
    constexpr bool Scripted(bool event, bool qte, bool codec, bool softEvent)
    { return event || qte || codec || softEvent; }
    constexpr bool CanThrow(unsigned state, bool alive, bool /*airborne*/, bool blocked, bool /*blade*/)
    {
        // Independent projectile release never changes the melee action or
        // animation. Ground/air attacks and Blade Mode can keep running.
        // Execution/forced animation graphs and story transitions retain input.
        return alive && !blocked && (state <= 0x7F ||
            (state >= 0x100000 && state < 0x100040) || state == 0x10007C);
    }
}
