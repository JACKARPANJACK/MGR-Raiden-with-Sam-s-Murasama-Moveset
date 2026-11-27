#pragma once
#include "CustomLightningEffects.h"
#include <Pl0000.h>
#include <cGame.h>
#include <atomic>
#include <memory>
#include <vector>
#include <algorithm>

// Particle lifecycle only. Every visible element is rendered by native ESP
// from EST records, textures and native parent bindings. No graphics draw calls.
class NativeLightningEffects
{
    struct Effect {
        cEspControler controller;
        unsigned ticks=0;
        bool charge=false;
        ~Effect(){reinterpret_cast<void(__thiscall*)(cEspControler*)>(shared::base+0xAAA9B0)(&controller);}
    };
    std::vector<std::unique_ptr<Effect>> effects;
    std::atomic<int> preview{-1};
    std::atomic<unsigned> accepted{0},failed{0};
    bool Spawn(Entity* anchor,int id,unsigned life,bool charge=false,int bone=-1)
    {
        if(effects.size()>=32 || !anchor)return false;
        auto effect=std::make_unique<Effect>();effect->charge=charge;effect->ticks=life;
        const bool ok=CustomLightningEffects::Spawn(anchor,id,&effect->controller,bone);
        if(ok){++accepted;effects.push_back(std::move(effect));}else ++failed;
        char path[MAX_PATH]{};GetModuleFileNameA(nullptr,path,MAX_PATH);
        if(char* slash=std::strrchr(path,'\\'))strcpy_s(slash+1,MAX_PATH-size_t(slash+1-path),"scripts\\LightningRuntime.log");
        FILE* file=nullptr;fopen_s(&file,path,"a");
        if(file){std::fprintf(file,"native-v3 atlas=900 bank=%X est=%d matrixBone=%d accepted=%u\n",CustomLightningEffects::Bank,id,bone,unsigned(ok));std::fclose(file);}
        return ok;
    }
public:
    static NativeLightningEffects& Get(){static NativeLightningEffects fx;return fx;}
    void QueuePreview(unsigned move,bool charge){preview.store(int(move)+(charge?100:0));}
    unsigned Accepted() const{return accepted.load();}
    unsigned Failed() const{return failed.load();}
    void EndCharge(){for(auto& e:effects)if(e->charge){e->controller.FadeUnits(2,0);e->ticks=(std::min)(e->ticks,3u);}}
    void Clear(){for(auto& e:effects){e->controller.FadeUnits(2,0);e->ticks=(std::min)(e->ticks,3u);}}
    void Shutdown(){preview=-1;effects.clear();}
    void Charge(Pl0000* player,unsigned move)
    {
        if(!player || !player->m_pEntity)return;EndCharge();
        Spawn(player->m_pEntity,CustomLightningEffects::Charge(move),100,true,13);
        if(move==24)Spawn(player->m_pEntity,CustomLightningEffects::Charge(move),100,true,9);
    }
    void Release(Pl0000* player,unsigned move,Entity* target=nullptr)
    {
        if(!player || !player->m_pEntity)return;EndCharge();
        const int id=move==21 && !target?CustomLightningEffects::Miss:CustomLightningEffects::Release(move);
        Spawn(target?target:player->m_pEntity,id,60);
    }
    void Impact(Entity* target){Spawn(target,CustomLightningEffects::Impact,25);}
    void Tick(Pl0000* player)
    {
        if(!player || !player->m_pEntity || !player->isAlive() || player->isBladeModeActive() ||
            g_StaFlags.STA_EVENT || g_StaFlags.STA_QTE || g_StaFlags.STA_CODEC || g_StaFlags.STA_SOFT_EVENT) {Clear();preview=-1;}
        if(g_StaFlags.STA_PAUSE)return;
        for(auto& e:effects)if(e->ticks)--e->ticks;
        effects.erase(std::remove_if(effects.begin(),effects.end(),[](const auto& e){return e->ticks==0;}),effects.end());
        const int request=preview.exchange(-1);
        if(player && request>=0){if(request>=100)Charge(player,unsigned(request-100));else Release(player,unsigned(request));}
    }
};
