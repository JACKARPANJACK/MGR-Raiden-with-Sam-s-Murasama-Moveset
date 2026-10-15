#pragma once
namespace StealthInputPolicy {
struct JumpLatch {
    bool previous=false,down=false,handled=false,active=false;
    void Reset(){*this={};}
    void Begin(){down=false;handled=false;active=true;}
    bool Observe(bool pressed) {
        if(!active)return false;
        down|=pressed;
        if(!pressed || previous || handled)return false;
        handled=true;return true;
    }
    void End(){if(active){previous=down;active=false;}}
};
template<class Input> inline void FilterJump(Input& input,unsigned binding,bool blocked=true) {
    if(!blocked)return;
    const unsigned mask=binding&~0xFu;
    input.m_On&=~mask;input.m_Trig&=~mask;input.m_Rep&=~mask;
}
}
