#pragma once
#include <cObjReadManager.h>
#include "SamVisualEffects.h"
#include "GunRuntimeLog.h"

class MonsoonKickEffects {
    bool requested=false,registered=false,playing=false;
    Hw::cFmerge archive;
    cEspControler kick;
public:
    static constexpr unsigned Bank=0x201A0;
    static constexpr int Number=16; // em01a0_2300_0_seq.bxm's native kick effect
    static MonsoonKickEffects& Get(){static MonsoonKickEffects value;return value;}
    bool Update() {
        const bool loaded=g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(Bank),0)!=FALSE;
        if(registered && loaded)return true;
        if(registered && !loaded){registered=false;requested=false;}
        if(!requested)requested=g_ObjReadManager.requestObject(static_cast<eObjID>(Bank),0)!=FALSE;
        if(!requested || !g_ObjReadManager.isObjectLoaded(static_cast<eObjID>(Bank),0) ||
            !g_ObjReadManager.getDataAtSet(archive,static_cast<eObjID>(Bank),0))return false;
        using Register=BOOL(__cdecl*)(unsigned,Hw::cFmerge*);
        registered=reinterpret_cast<Register>(shared::base+0xA00C50)(Bank,&archive)!=FALSE;
        if(registered)GunRuntimeLog::Write("[MonsoonKick] native effect bank registered bank=%X effect=%d",Bank,Number);
        return registered;
    }
    void Play(Pl0000* player) {
        if(!player || !player->m_pEntity || !Update())return;
        kick.FadeUnits(1,0);
        // Use Monsoon's explicit bank; Raiden's effect 16 is a different asset.
        SamVisualEffects::Spawn(player,player->m_pEntity,Bank,Number,&kick);
        playing=true;
    }
    void Stop(){if(playing){kick.FadeUnits(3,0);playing=false;}}
};
