#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace SamUltimatePolicy
{
    constexpr unsigned ControllerButton=0x20, SelectButton=0x200, LeftStickButton=0x1000, RightStickButton=0x8000;
    inline bool Press(unsigned on,unsigned trig,unsigned previous)
    { return ((on|trig)&ControllerButton)!=0 && (previous&ControllerButton)==0; }
    // Unused/default case in both native Sam input and action dispatch tables.
    constexpr uint32_t Action = 0x10007Cu;
    struct Move { std::string name; std::string windup; std::string release; bool raiden=false; };
    inline std::vector<Move> Moves = {
        {"Boss stone burst", "3020", "3024"},
        {"Boss charged slash", "3000", "3004"},
        {"Boss sonic slash", "3010", "3014"},
        {"Boss sweeping finisher", "", "2600"},
        {"Boss leaping slash", "3300", "3302"},
        {"Sam rapid slashes", "9100", "9108"},
        {"Boss sweeping combo", "2600", "2610"},
        {"Boss Judgement Cut", "92e0", "92e4"},
        {"Raiden thunder slice", "", "2400", true},
        {"Raiden lightning storm", "", "3501", true},
        {"Boss unarmed grab & smash", "a648", "a649"},
        {"Boss unarmed tackle & smash", "a646", "a648"},
        {"Boss Murasama round trip throw", "3200", "3210"},
        {"Raiden lightning draw slash", "2420", "2422", true},
    };
    inline unsigned Count() { return Moves.size(); }
    // Boss attacks belong to directional add-ons. Only Raiden specials use X.
    inline std::vector<unsigned> UltimateIndices = {8, 9, 13};
    inline unsigned UltimateCount() { return UltimateIndices.size(); }
    inline unsigned UltimateIndex(unsigned selection) { return UltimateCount() ? UltimateIndices[selection % UltimateCount()] : 0; }
    inline bool Attack(uint32_t state)
    {
        return state >= 0x10000F && state <= 0x10001A;
    }
    inline bool Neutral(uint32_t state)
    {
        return state == 0x100000 || state == 0x100001 || state == 0x100002;
    }
    inline bool ReserveButton(unsigned state,bool alive,bool airborne,bool blocked,bool blade,bool subweapon)
    { return alive && !airborne && !blocked && !blade && !subweapon && (Neutral(state)||Attack(state)||state==Action); }
    // Never steal aerial, damage, execution, or scripted states.
    inline bool CanStart(uint32_t state, bool alive, bool airborne, bool finished)
    {
        return alive && !airborne && (Neutral(state) || (Attack(state) && finished));
    }
    struct Queue
    {
        bool pending = false;
        unsigned ticks = 0;
        void Reset() { pending = false; ticks = 0; }
        void Request() { pending = true; ticks = 0; }
        void Tick(uint32_t state, bool alive, bool airborne)
        {
            if (!pending) return;
            if (!alive || airborne || (!Neutral(state) && !Attack(state)) || ++ticks > 360)
                Reset();
        }
    };
}
