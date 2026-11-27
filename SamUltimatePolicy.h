#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "LightningSpecialPolicy.h"

namespace SamUltimatePolicy
{
    constexpr unsigned ControllerButton=0x20, SelectButton=0x200, LeftStickButton=0x1000, RightStickButton=0x8000;
    inline bool Press(unsigned on,unsigned trig,unsigned previous)
    { return ((on|trig)&ControllerButton)!=0 && (previous&ControllerButton)==0; }
    // Unused/default case in both native Sam input and action dispatch tables.
    constexpr uint32_t Action = 0x10007Cu;
    struct Move {
        std::string name; std::string windup; std::string release;
        bool raiden=false;
        bool playable=false;
        std::vector<std::string> followups{};
        const std::string& Stage(unsigned index) const {
            if (!windup.empty()) { if (index == 0) return windup; --index; }
            return index == 0 ? release : followups.at(index - 1);
        }
        std::vector<std::string> Stages() const {
            std::vector<std::string> stages;
            if (!windup.empty()) stages.push_back(windup);
            stages.push_back(release);
            stages.insert(stages.end(), followups.begin(), followups.end());
            return stages;
        }
    };
    inline std::vector<Move> Moves = {
        {"Boss stone burst", "3020", "3024"},
        {"Boss charged slash", "3000", "3004"},
        {"Boss sonic slash", "3010", "3014"},
        {"Boss sweeping finisher", "", "2600"},
        {"Boss leaping slash", "3300", "3302"},
        {"Sam rapid/spinning slashes", "9100", "9101", false, true, {"9102", "9108"}},
        {"Boss sweeping combo", "2600", "2610"},
        {"Boss Judgement Cut", "92e0", "3015"},
        {"Raiden thunder slice", "", "2400", true},
        {"Raiden lightning storm", "", "3501", true},
        {"Boss unarmed grab & smash", "a648", "a649"},
        {"Boss unarmed tackle & smash", "a646", "a648"},
        {"Boss Murasama round trip throw", "3200", "3210"},
        {"Raiden lightning draw slash", "2420", "2422", true},
        {"Boss forward evade slash", "", "2510"},
        {"Boss backward evade slash", "", "2520"},
        {"Boss left evade slash", "", "2530"},
        {"Boss right evade slash", "", "2540"},
        {"Boss advancing flurry", "", "2220"},
        {"Sam template light combo extension", "", "9000", false, true, {"9001", "9002", "9003"}},
    };
    inline Move LightExtension()
    { return {"Sam template light combo extension","","9000",false,true,{"9001","9002","9003"}}; }
    inline std::vector<unsigned> UltimateIndices = {8, 9, 13};
    inline void EnsureLightningSpecials()
    {
        // Upgrade the old aerial/kick placeholders, including legacy INIs.
        // Leave explicitly different user-authored animation mappings alone.
        if(Moves.size()>13) {
            if(Moves[8].raiden && Moves[8].release=="2400")Moves[8]={"Raiden thunder slice","3010","3014"};
            if(Moves[9].raiden && Moves[9].release=="3501")Moves[9]={"Raiden lightning storm","9100","9108",false,true};
            if(Moves[13].raiden && Moves[13].release=="2422")Moves[13]={"Raiden lightning draw slash","3000","3004"};
        }
        const Move specials[] = {
            {"Chidori", "3000", "3004"},
            {"Kirin", "3020", "3024"},
            {"Chidori Nagashi", "9100", "9108", false, true},
            {"Chidori Senbon", "3010", "3014"},
            {"Double Chidori", "9100", "9101", false, true},
            {"Chidori Sharp Spear", "3000", "3004"},
            {"Chidori sword wave", "3010", "3014"},
            {"MGS4 lightning discharge", "9100", "9108", false, true}
        };
        if(Moves.size()<28) Moves.resize(28);
        for(unsigned i=0;i<8;++i) Moves[20+i]=specials[i];
        for(unsigned i=20;i<28;++i)
            if(std::find(UltimateIndices.begin(),UltimateIndices.end(),i)==UltimateIndices.end())
                UltimateIndices.push_back(i);
    }
    inline unsigned Count() { return Moves.size(); }
    // Boss attacks belong to directional add-ons. Only Raiden specials use X.
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
    inline bool Running(uint32_t state)
    {return state==0x100008 || state==0x100009 || state==0x100014 || state==0x100015 || state==0x100016;}
    inline bool ReserveButton(unsigned state,bool alive,bool airborne,bool blocked,bool blade,bool subweapon)
    { return alive && !airborne && !blocked && !blade && !subweapon && !Running(state) && (Neutral(state)||Attack(state)||state==Action); }
    // Never steal aerial, damage, execution, or scripted states.
    inline bool CanStart(uint32_t state, bool alive, bool airborne, bool finished,bool freshAttack=false)
    {
        return alive && !airborne && !Running(state) && (Neutral(state) || (Attack(state) && (finished || freshAttack)));
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
