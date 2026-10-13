#pragma once
#include <Pl0000.h>
#include <BehaviorAppBase.h>
#include <BehaviorEmBase.h>
#include <CharacterControl.h>
#include "SamTemplateExecutionPolicy.h"
#include "SamStagePolicy.h"
#include "SamBossSequence.h"
#include "SheathController.h"
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <windows.h>

class SamTemplateExecution
{
    struct Clip { std::vector<unsigned char> motion,sequence; };
    std::map<std::string,Clip> clips;
    EntityHandle victim;
    Pl0000* owner=nullptr;
    SamTemplateExecutionPolicy::Route route=SamTemplateExecutionPolicy::Route::Grab;
    unsigned stage=0,ticks=0;
    unsigned savedEnemy[4]{};
    EntityHandle savedVictimExecutor,savedPlayerExecutor;
    bool active=false;

    static std::vector<unsigned char> Read(const std::string& name)
    {
        char exe[MAX_PATH]{};GetModuleFileNameA(nullptr,exe,MAX_PATH);
        char* slash=std::strrchr(exe,'\\');if(!slash)return {};
        *slash=0;const std::string path=std::string(exe)+"\\scripts\\SamTemplateAnims\\"+name;
        FILE* file=nullptr;fopen_s(&file,path.c_str(),"rb");if(!file)return {};
        std::fseek(file,0,SEEK_END);const long size=std::ftell(file);std::rewind(file);
        if(size<=0 || size>4*1024*1024){std::fclose(file);return {};}
        std::vector<unsigned char> data(size);
        const bool ok=std::fread(data.data(),1,data.size(),file)==data.size();std::fclose(file);
        return ok ? data : std::vector<unsigned char>{};
    }
    Clip* Load(const char* side,const char* code)
    {
        const std::string key=std::string(side)+"_"+code;
        auto it=clips.find(key);if(it!=clips.end())return &it->second;
        Clip clip{Read(key+".mot"),Read(key+"_seq.bxm")};
        if(clip.motion.size()<12 || !SamStagePolicy::Length(clip.motion.data()) ||
            clip.sequence.size()<16 || SamBossSequence::Size(clip.sequence.data())!=clip.sequence.size())return nullptr;
        return &clips.emplace(key,std::move(clip)).first->second;
    }
    static float Remaining(Behavior* actor)
    { return reinterpret_cast<float(__thiscall*)(Behavior*,int)>(shared::base+0x695BD0)(actor,0); }
    bool Play()
    {
        Entity* target=victim.getEntity();if(!target || !target->m_pBehavior)return false;
        const char* code=SamTemplateExecutionPolicy::Code(route,stage);
        Clip* attacker=Load("attacker",code);Clip* enemy=Load("victim",code);
        if(!attacker || !enemy)return false;
        if(owner->setDirectAnimation(attacker->motion.data(),attacker->sequence.data(),0,0,1,0x8100000,0,1)==-1)return false;
        SheathController::Instance().AnimateSheathDirect(owner,code,1);
        return target->m_pBehavior->setDirectAnimation(enemy->motion.data(),enemy->sequence.data(),0,0,1,0x8038000,0,1)!=-1;
    }
    void Align()
    {
        Entity* target=victim.getEntity();if(!target || !target->m_pBehavior)return;
        // Template AdjustQTEPosition: the attacker MOT's 0xF00 marker gives
        // the victim's relative position/rotation, transformed by the player.
        Hw::cVec4 offset{},rotation{};
        reinterpret_cast<void(__thiscall*)(Behavior*,Hw::cVec4*,Hw::cVec4*)>(shared::base+0x68CE90)(owner,&offset,&rotation);
        D3DXVECTOR3 world;D3DXVec3TransformNormal(&world,reinterpret_cast<D3DXVECTOR3*>(&offset),reinterpret_cast<D3DXMATRIX*>(&owner->m_LocalMatrix));
        Hw::cVec4 pos{world.x+owner->m_LocalMatrix._41,world.y+owner->m_LocalMatrix._42,world.z+owner->m_LocalMatrix._43,1};
        rotation.y=Hw::RadAdjust(owner->m_Rot.y+rotation.y);
        target->m_pBehavior->setRotation(rotation);target->m_pBehavior->setTransPos(pos);
        if(target->m_pBehavior->m_pCharacterControl)target->m_pBehavior->m_pCharacterControl->setPosition(pos,TRUE);
        target->setTransPos(pos);
    }
public:
    static SamTemplateExecution& Get(){static SamTemplateExecution value;return value;}
    bool Active(Pl0000* player)const{return active && owner==player;}
    const char* CurrentCode()const{return active ? SamTemplateExecutionPolicy::Code(route,stage) : "";}
    bool Start(Pl0000* player,Entity* target,SamTemplateExecutionPolicy::Route selection)
    {
        if(active || !player || !player->m_pEntity || !target || !target->m_pBehavior || !target->isAlive())return false;
        auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);
        const auto delta=target->getTransPos()-player->m_pEntity->getTransPos();
        Entity* executor=enemy->m_QTETargetExecutor.getEntity();
        if(!SamExecutionPolicy::Eligible(unsigned(target->m_ObjId),enemy->m_Hp,enemy->m_HpMax,delta.length(),true,
            executor && executor!=player->m_pEntity) || player->isInAir() || player->isBladeModeActive() || !player->isAlive())return false;
        for(unsigned i=0;i<SamTemplateExecutionPolicy::Stages(selection);++i)
            if(!Load("attacker",SamTemplateExecutionPolicy::Code(selection,i)) || !Load("victim",SamTemplateExecutionPolicy::Code(selection,i)))return false;
        owner=player;victim=target;route=selection;stage=ticks=0;
        savedEnemy[0]=enemy->m_Rno0;savedEnemy[1]=enemy->m_Rno1;savedEnemy[2]=enemy->m_Rno2;savedEnemy[3]=enemy->m_Rno3;
        savedVictimExecutor=enemy->m_QTETargetExecutor;savedPlayerExecutor=player->m_QTETargetExecutor;
        enemy->m_TransSpeed={0,0,0,0};player->m_TransSpeed={0,0,0,0};player->field_1020={0,0,0,0};
        enemy->m_QTETargetExecutor=player->m_pEntity;player->m_QTETargetExecutor=target;
        enemy->setRno(SamTemplateExecutionPolicy::VictimAction(route),10,0,0);
        player->setRno(SamTemplateExecutionPolicy::Action,0,0,0);active=true;
        if(!Play()){Abort(player);return false;}
        Align();return true;
    }
    void Abort(Pl0000* player,bool execute=false)
    {
        if(!Active(player))return;
        Entity* target=victim.getEntity();active=false;
        if(target && target->m_pBehavior)
        {
            auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);
            if(target->isAlive() && enemy->m_Rno0==SamTemplateExecutionPolicy::VictimAction(route)) {
                enemy->setRno(savedEnemy[0],savedEnemy[1],savedEnemy[2],savedEnemy[3]);
                if(execute && target->isAlive())enemy->damage(enemy->m_Hp,false);
                else { enemy->m_TransSpeed={0,0,0,0};static_cast<BehaviorEmBase*>(enemy)->setWait(); }
            }
            if(enemy->m_QTETargetExecutor.getEntity()==player->m_pEntity)enemy->m_QTETargetExecutor=savedVictimExecutor;
        }
        if(player->m_QTETargetExecutor.getEntity()==target)player->m_QTETargetExecutor=savedPlayerExecutor;
        if(player->isAlive() && player->m_Rno0==SamTemplateExecutionPolicy::Action) {
            player->setRno(player->isInAir()?0x100005:0x100000,0,0,0);
            if(!player->isInAir())player->requestAnimationByMap(4);
        }
        owner=nullptr;victim=nullptr;
    }
    void Tick(Pl0000* player)
    {
        if(!Active(player))return;
        Entity* target=victim.getEntity();
        if(!target || !target->m_pBehavior || !target->isAlive() || !player->isAlive() ||
            player->isInAir() || player->isBladeModeActive() || player->m_Rno0!=SamTemplateExecutionPolicy::Action ||
            target->m_pBehavior->m_Rno0!=SamTemplateExecutionPolicy::VictimAction(route) || ++ticks>1800)
        {Abort(player);return;}
        auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);enemy->m_TransSpeed={0,0,0,0};
        reinterpret_cast<void(__thiscall*)(Behavior*,float,float)>(shared::base+0x6C80A0)(enemy,1,1);
        Align();
        if(SamTemplateExecutionPolicy::Completed(Remaining(player),Remaining(enemy))) {
            if(++stage<SamTemplateExecutionPolicy::Stages(route)){if(!Play())Abort(player);}
            else Abort(player,true);
        }
    }
};
