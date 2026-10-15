#pragma once
namespace SamAttackInputPolicy
{
    struct Snapshot {bool light=false,heavy=false,lightHeld=false,heavyHeld=false;};
    struct Reader
    {
        unsigned previous=0;
        void Reset(){previous=0;}
        Snapshot Poll(unsigned on,unsigned trig,unsigned lightBinding,unsigned heavyBinding)
        {
            // Native bindings can contain low direction bits as modifiers.
            // Direction changes are not attack-button presses.
            const unsigned light=lightBinding&~0xFu,heavy=heavyBinding&~0xFu;
            const unsigned edges=trig|(on&~previous);previous=on;
            return {bool(edges&light),bool(edges&heavy),bool(on&light),bool(on&heavy)};
        }
    };
    inline bool ChargeWait(bool windup,bool heavyHeld,unsigned ticks)
    {return windup && heavyHeld && ticks<90;}
}
