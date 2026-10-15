#pragma once
#include "StealthStancePolicy.h"
namespace StealthMotionPolicy {
constexpr unsigned Action=0x10007Au;
enum class Move {None,Dive,RollLeft,RollRight};
struct Direction {float x,z;};
inline Direction DiveDirection(float yaw,float cameraX,float cameraZ,float stickX,float stickY) {
    const Direction facing{std::sin(yaw),std::cos(yaw)};
    if(std::hypot(stickX,stickY)<.01f)return facing;
    const float length=std::hypot(cameraX,cameraZ);
    const Direction forward=length>.001f?Direction{cameraX/length,cameraZ/length}:facing;
    Direction result{forward.x*stickY+forward.z*stickX,forward.z*stickY-forward.x*stickX};
    const float size=std::hypot(result.x,result.z);
    return size>.001f?Direction{result.x/size,result.z/size}:facing;
}
inline bool BlockNativeJump(StealthStancePolicy::Stance stance,bool safeGround,bool evasive) {
    return stance!=StealthStancePolicy::Stance::Standing && (safeGround || evasive);
}
inline Move Request(StealthStancePolicy::Stance stance,bool jumpPress,int side,bool allowed) {
    if(!allowed || !jumpPress)return Move::None;
    if(stance==StealthStancePolicy::Stance::Crouch)return Move::Dive;
    if(stance==StealthStancePolicy::Stance::Prone && side)return side<0?Move::RollLeft:Move::RollRight;
    return Move::None;
}
inline int Clip(Move move,unsigned stage) {
    return move==Move::Dive?(stage?5:4):move==Move::RollLeft?6+int((std::min)(stage,2u)):9+int((std::min)(stage,2u));
}
inline float Speed(Move move,unsigned stage,unsigned ticks) {
    if(move==Move::Dive)return ticks<30?.13f*(1.0f-float(ticks)/45):.035f;
    return stage==1?.075f:.012f;
}
inline float Vertical(unsigned ticks,bool grounded) {
    return grounded&&ticks>6?0.0f:(std::max)(-.30f,.14f-float(ticks)*.012f);
}
}
