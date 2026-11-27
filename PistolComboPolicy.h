#pragma once
#include <cmath>
#include <array>
#include "PistolAttackCatalog.generated.h"
#include "PistolGunFuFeature.h"

namespace PistolComboPolicy {
constexpr unsigned Action=0x143;
enum Direction {None,Forward,Back,Left,Right};
inline Direction Resolve(float x,float y) {
    if(!std::isfinite(x+y))return None;
    if(std::fabs(y)>=.5f && std::fabs(y)>std::fabs(x)*1.35f)return y>0?Forward:Back;
    if(std::fabs(x)>=.5f && std::fabs(x)>std::fabs(y)*1.35f)return x<0?Left:Right;
    return None;
}
constexpr unsigned Length(Direction d) {return d==None?0u:1u;}
constexpr unsigned Clip(Direction d,unsigned step) {
    return d==Back?PistolAttackCatalog::Back[step%4]:
        d==Left?PistolAttackCatalog::Left[step%6]:
        d==Right?PistolAttackCatalog::Right[step%6]:
        PistolAttackCatalog::Forward[step%6];
}
// Shared between the input/action controller and the native pistol emitter.
inline bool button=false;
inline unsigned shots=0;
inline float x=0,y=0;
struct Input {
    bool held=false,armed=false,combo=false,gunFu=false,manual=false,fire=false;
    Direction previousDirection=None;
    Direction request=None;
    void Tick(bool buttonHeld,float horizontal,float vertical,bool enabled,bool active,bool trigger=false,bool directionalCombos=true) {
        const bool pressed=buttonHeld && !held;
        held=buttonHeld;request=None;fire=false;
        if(!enabled){armed=false;combo=buttonHeld;gunFu=manual=false;previousDirection=None;return;}
        if(!held){armed=true;combo=manual=false;previousDirection=None;return;}
        if(!armed && !gunFu)return;
        if(trigger){manual=fire=armed=true;combo=gunFu=false;previousDirection=None;return;}
        if(manual)return; // A regular firing hold remains aim while moving.
        if(!directionalCombos) {
            combo=gunFu=false;previousDirection=None;return;
        }
        Direction direction=Resolve(horizontal,vertical);
        // Diagonal WASD/stick input is a gun-fu command too. The dominant
        // axis selects the route; ties select forward/back.
        if(direction==None && std::isfinite(horizontal+vertical)) {
            if(std::fabs(vertical)>=.5f && std::fabs(vertical)>=std::fabs(horizontal))direction=vertical>0?Forward:Back;
            else if(std::fabs(horizontal)>=.5f)direction=horizontal>0?Right:Left;
        }
        if(direction!=None && ((!combo && !active) || (active && (pressed || direction!=previousDirection)))) {
            combo=gunFu=true;request=direction;
        }
        previousDirection=direction;
        if(active)combo=gunFu=true;
        else if(!combo && direction==None)gunFu=false;
    }
    Direction Take(){const auto result=request;request=None;return result;}
    bool Aim() const {return held && armed && !combo && !gunFu;}
    void Cancel(){request=None;combo=held;armed=!held;}
    void ManualHandoff(){request=None;combo=gunFu=false;manual=armed=true;fire=false;}
};
inline Input input;
struct Chain {
    struct Step {Direction direction=None;unsigned clip=0;float queuedAt=0;};
    static constexpr unsigned Capacity=1;
    static constexpr float BufferFrames=24; // 0.4 seconds, independent of playback rate/FPS.
    std::array<Step,Capacity> pending{};
    std::array<unsigned,5> phase{};
    unsigned head=0,count=0;
    void Reset(){*this={};}
    void Clear(){head=count=0;} // Retain variation cursors between completed routes.
    bool Push(Direction direction,float now=0) {
        if(direction==None || !std::isfinite(now))return false;
        if(count && pending[head].direction==direction) {pending[head].queuedAt=now;return true;}
        const unsigned index=unsigned(direction);
        pending[head]={direction,Clip(direction,phase[index]),now};
        ++phase[index];count=1;return true; // Latest direction wins; never queue seconds of unwanted moves.
    }
    bool Pop(Step& step,float now=0) {
        if(!count)return false;
        if(now-pending[head].queuedAt>BufferFrames){Clear();return false;}
        step=pending[head];--count;return true;
    }
};
struct Point {float x,y,z;};
inline bool BarrelTarget(Point origin,Point forward,Point& target) {
    const float length=std::sqrt(forward.x*forward.x+forward.y*forward.y+forward.z*forward.z);
    if(!std::isfinite(length) || length<.0001f)return false;
    const float scale=100.0f/length;
    target={origin.x+forward.x*scale,origin.y+forward.y*scale,origin.z+forward.z*scale};
    return std::isfinite(target.x+target.y+target.z);
}
struct Playback {
    unsigned ticks=0,stalled=0,next=0,due=0;
    float previous=-1;
    enum Result {Playing,Fire,Finished,Failed};
    void Reset(){*this={};}
    Result Tick(float frame,const unsigned* cues,unsigned count,unsigned recovery,unsigned frames,bool paused=false) {
        due=0;
        if(paused)return Playing;
        if(!std::isfinite(frame) || frame<0 || ++ticks>frames+8)return Failed;
        if(previous>=0 && frame<=previous+.001f)++stalled;else stalled=0;
        previous=frame;
        if(stalled>=8)return Failed;
        if(frame>=frames)return Finished; // No backlog after an invalid/teleported clock.
        while(next<count && frame>=cues[next]){++next;++due;}
        if(due)return Fire;
        if(frame>=recovery)return Finished;
        return Playing;
    }
};

}
