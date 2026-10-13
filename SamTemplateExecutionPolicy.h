#pragma once
#include "SamExecutionPolicy.h"
namespace SamTemplateExecutionPolicy
{
    constexpr unsigned Action=0x10007D;
    enum class Route { Grab, Stab, Sweep, Counter };
    inline Route ForMove(unsigned move)
    { return move==10 || move==11 ? Route::Grab : move==3 ? Route::Sweep : Route::Stab; }
    inline const char* Code(Route route,unsigned stage=0)
    {
        switch(route) {
        case Route::Grab:return stage ? "a649" : "a648";
        case Route::Stab:return "92b0"; // template map 1340 / action 0x100501
        case Route::Sweep:return "92c0"; // template map 1339 / action 0x100500
        default:return "92e4"; // available template map 1338 / action 0x100502
        }
    }
    inline unsigned Stages(Route route) { return route==Route::Grab ? 2u : 1u; }
    inline unsigned VictimAction(Route route)
    { return route==Route::Sweep ? 0x100500 : route==Route::Stab ? 0x100501 : 0x100502; }
    inline bool Completed(float attackerRemaining,float victimRemaining)
    { return attackerRemaining<=0.001f && victimRemaining<=0.001f; }
}
