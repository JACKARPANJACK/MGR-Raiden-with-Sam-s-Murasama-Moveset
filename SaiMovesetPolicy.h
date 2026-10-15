#pragma once

namespace SaiMovesetPolicy {
    // Native Raiden action handlers, not Combo List menu IDs. wp2030.dat
    // supplies their MOT/sequence pairs and weapon hit handling.
    constexpr unsigned StabbingDespair = 0xA1, ForceOfMagnetism = 0xA2;
    constexpr unsigned ThrowRecovery = 0x9F, ForceRecovery = 0xA3;
    constexpr unsigned DoubleThrowStab = 0xA0, SpinSlice = 0xA4;
    constexpr unsigned Sommersault = 0xA5, UnavoidableFate = 0x9E;
    constexpr unsigned BufferTicks = 24;

    inline unsigned FollowUp(unsigned state, bool airborne) {
        if (airborne) return 0;
        switch (state) {
        case ThrowRecovery: return DoubleThrowStab; // Combo List 36
        case ForceRecovery: return SpinSlice;      // Combo List 38
        case SpinSlice: return Sommersault;        // Combo List 39
        default: return 0;
        }
    }
    inline unsigned AirAttack(unsigned state, bool airborne) {
        // Native jump/free-fall only; never cancel damage, QTEs or air attacks.
        return airborne && (state == 0xA || state == 0xB) ? UnavoidableFate : 0;
    }
    struct Buffer {
        unsigned state = 0, ticks = 0;
        void Reset() { state = ticks = 0; }
        unsigned Poll(unsigned action, bool airborne, bool heavy, bool light,
                      bool initialized, float frame) {
            const unsigned follow = FollowUp(action, airborne);
            if (light || !follow || (state && state != action)) Reset();
            if (light) return 0;
            if (heavy && follow) { state = action; ticks = BufferTicks; }
            if (!ticks) return heavy ? AirAttack(action, airborne) : 0;
            // Let the native handler initialize its clip and deliver the first
            // strike before accepting a buffered follow-up.
            if (initialized && frame >= 8.0f) { Reset(); return follow; }
            if (--ticks == 0) Reset();
            return 0;
        }
    };
}
