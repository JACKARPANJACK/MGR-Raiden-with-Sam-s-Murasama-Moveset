#pragma once
#include <cmath>

namespace SamDirectionalPolicy
{
    inline bool NativeSprint(unsigned action, bool runHeld)
    {
        return runHeld || action == 0x100008 || action == 0x100009 ||
            action == 0x100014 || action == 0x100015 || action == 0x100016;
    }
    enum Direction { None, Forward, Back, Left, Right };
    inline Direction Resolve(float x, float y)
    {
        if (std::fabs(x) < 0.40f && std::fabs(y) < 0.40f) return None;
        if (!std::isfinite(x) || !std::isfinite(y)) return None;
        // Diagonals have no unique directional command. Do not guess an axis.
        if (std::fabs(y) > std::fabs(x) * 1.35f) return y > 0 ? Forward : Back;
        if (std::fabs(x) > std::fabs(y) * 1.35f) return x < 0 ? Left : Right;
        return None;
    }
    // Indices into the shared stage catalog, excluded from the X ultimate set.
    inline int heldLight[] = {-1, 14, 3, 16, 17};
    inline int heldHeavy[] = {-1, 1, 0, 15, 18};
    inline int flickLight[] = {-1, 5, 10, 6, 11};
    // Judgement Cut is reserved for a complete stick circle, never a forward tap.
    inline int flickHeavy[] = {-1, 1, 12, 2, 4};
    inline int Move(Direction direction, bool heavy, bool flick)
    {
        if (direction == None) return -1;
        return (flick ? (heavy ? flickHeavy : flickLight) :
                        (heavy ? heldHeavy : heldLight))[direction];
    }
    struct Input
    {
        Direction held = None, flick = None;
        unsigned flickTicks = 0;
        bool centered = true;
        float accumAngle = 0.0f;
        float prevAngle = 0.0f;
        bool hasPrevAngle = false;
        unsigned flick360Ticks = 0;
        unsigned neutralTicks = 0;
        unsigned rotationTicks = 0;

        void Reset() { *this = {}; }

        int Poll(float x, float y, bool light, bool heavy, bool allowRotation = true)
        {
            held = Resolve(x, y);
            const float r = std::sqrt(x * x + y * y);
            const bool neutral = r < 0.25f;
            if (!std::isfinite(r) || (light && heavy))
            { Reset(); centered = neutral; return -1; }
            if (!neutral && held == None) { flick = None; flickTicks = 0; centered = false; }
            if (!allowRotation) { hasPrevAngle = false; accumAngle = 0; flick360Ticks = 0; rotationTicks = 0; }
            // A fresh direction supersedes a released-stick flick immediately.
            if (held != None && flick != None && held != flick)
            { flick = None; flickTicks = 0; }

            if (flickTicks && --flickTicks == 0) flick = None;
            if (flick360Ticks && --flick360Ticks == 0) {}

            if (neutral)
            {
                centered = true;
                ++neutralTicks;
                hasPrevAngle = false;
                accumAngle = 0.0f;
                rotationTicks = 0;
            }
            else
            {
                neutralTicks = 0;
                if (allowRotation && r >= 0.45f)
                {
                    const float angle = std::atan2(y, x);
                    if (hasPrevAngle)
                    {
                        if (++rotationTicks > 30) { accumAngle = 0; rotationTicks = 0; }
                        float delta = angle - prevAngle;
                        constexpr float pi = 3.14159265358979323846f;
                        while (delta > pi) delta -= 2.0f * pi;
                        while (delta < -pi) delta += 2.0f * pi;
                        if (std::fabs(delta) > pi * 0.5f + 0.05f || accumAngle * delta < 0.0f)
                        {
                            accumAngle = 0.0f;
                            rotationTicks = 0;
                        }
                        else
                        {
                            accumAngle += delta;
                        }
                        if (std::fabs(accumAngle) >= 2.0f * pi - 0.05f)
                        {
                            flick360Ticks = 30;
                            accumAngle = 0.0f;
                        }
                    }
                    prevAngle = angle;
                    hasPrevAngle = true;
                }
            }

            if (centered && held != None)
            {
                flick = held;
                flickTicks = 8;
                centered = false;
            }

            if (light == heavy) return -1; // simultaneous buttons retain native dodge

            // 360-degree stick flick + Heavy attack performs Judgement Cut charge (Move 7)
            if (heavy && flick360Ticks > 0)
            {
                flick360Ticks = 0;
                flickTicks = 0;
                flick = None;
                return 7;
            }

            const int result = Move(flickTicks ? flick : held, heavy, flickTicks != 0);
            if (light) flick360Ticks = 0;
            if (result >= 0) { flickTicks = 0; flick = None; }
            return result;
        }
    };

    // Combat commands use explicit button strings. The older gesture Input
    // remains available to the gun's flick-to-spray recognizer.
    constexpr unsigned ComboWindow = 36;
    inline int Single(Direction direction, bool heavy)
    {
        constexpr int lightMoves[] = {-1,14,3,16,17};
        constexpr int heavyMoves[] = {-1,1,0,15,18};
        return direction >= Forward && direction <= Right ?
            (heavy ? heavyMoves : lightMoves)[direction] : -1;
    }
    inline int Combo(Direction direction, bool firstHeavy, bool secondHeavy)
    {
        constexpr int ll[] = {-1,5,6,11,4};
        constexpr int lh[] = {-1,13,10,2,9};
        constexpr int hl[] = {-1,8,12,-1,-1};
        if(direction < Forward || direction > Right || (firstHeavy && secondHeavy))return -1;
        return (firstHeavy ? hl : secondHeavy ? lh : ll)[direction];
    }
    struct CombatInput
    {
        Input gesture;
        Direction chainDirection=None;
        unsigned chainTicks=0;
        bool firstHeavy=false, chained=false;
        void ClearChain(){chainDirection=None;chainTicks=0;firstHeavy=false;}
        void Reset(){*this={};}
        int Poll(float x,float y,bool light,bool heavy,bool allowRotation=true)
        {
            chained=false;
            gesture.Poll(x,y,false,false,allowRotation);
            const Direction direction=Resolve(x,y);
            if(!std::isfinite(x) || !std::isfinite(y) || (light && heavy)) {Reset();return -1;}
            if(chainTicks && --chainTicks==0)ClearChain();
            if(direction==None || direction!=chainDirection)ClearChain();
            if(light==heavy)return -1;
            if(heavy && gesture.flick360Ticks) {
                Reset();return 7;
            }
            if(light)gesture.flick360Ticks=0;
            if(direction==None)return -1; // released directions never select a special
            if(chainTicks) {
                const int move=Combo(direction,firstHeavy,heavy);
                ClearChain();
                if(move>=0){chained=true;return move;}
            }
            chainDirection=direction;chainTicks=ComboWindow;firstHeavy=heavy;
            return Single(direction,heavy);
        }
    };
}
