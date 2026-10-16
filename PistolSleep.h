#pragma once
#include "PistolPolicy.h"
#include "SamTargets.h"
#include "GunRuntimeLog.h"
#include "PistolNative.h"
#include <BehaviorEmBase.h>
#include <cSlowRateManager.h>
#include <memory>
#include <vector>

// Sleep owns an actor-local rate unit, never a shared enemy/world slow unit.
// Native knockdown plays to its ground loop before the AI clock is suspended.
class PistolSleep {
    struct Victim {
        EntityHandle handle;PistolPolicy::Sedation dose;
        cSlowRate original;cSlowRateUnit* unit=nullptr;
        unsigned remaining=0,fallTicks=0;int hp=0;
    };
    std::vector<std::unique_ptr<Victim>> victims;
    static void Wake(Victim& v) {
        if(auto* entity=v.handle.getEntity();entity && v.unit && entity->getSlowRate()->m_pUnit==v.unit)
            *entity->getSlowRate()=v.original;
        if(auto* entity=v.handle.getEntity();entity && entity->isAlive() && v.remaining && entity->m_pBehavior) {
            auto* actor=entity->m_pBehavior;
            if(actor->m_Rno0==PistolNative::DownState && actor->m_Rno1==PistolNative::GroundLoop)
                actor->m_Rno1=PistolNative::GetUp;
        }
        v.original.cleanup();v.unit=nullptr;v.remaining=0;v.dose.Reset();
    }
public:
    void Reset() {for(auto& v:victims)Wake(*v);victims.clear();}
    void Hit(Entity* target) {
        if(!SamTargets::Enemy(target) || !SamElectricPolicy::Ordinary(unsigned(target->m_ObjId)))return;
        Victim* victim=nullptr;
        for(auto& v:victims)if(v->handle.getEntity()==target){victim=v.get();break;}
        if(!victim){auto v=std::make_unique<Victim>();v->handle=target;victim=v.get();victims.push_back(std::move(v));}
        if(victim->remaining)return;
        auto* actor=static_cast<BehaviorEmBase*>(target->m_pBehavior);
        const bool asleep=victim->dose.Hit(actor->m_HpMax);
        GunRuntimeLog::Write("M9 dose target=%p hits=%u required=%u",target,victim->dose.hits,PistolPolicy::SleepHits(actor->m_HpMax));
        if(!asleep)return;
        if(!PistolNative::KnockDown(actor))return;
        victim->remaining=PistolPolicy::SleepTicks;victim->hp=actor->m_Hp;
        victim->fallTicks=0;
        actor->m_TransSpeed=Hw::cVec4(0,0,0,0);
        if(actor->m_pCharacterControl && actor->m_pCharacterControl->m_pVelocity)
            *actor->m_pCharacterControl->m_pVelocity=Hw::cVec4(0,0,0,0);
        GunRuntimeLog::Write("M9 knockdown target=%p hits=%u state=%X",target,victim->dose.hits,actor->m_Rno0);
    }
    void Tick(Pl0000* player,bool advance) {
        for(auto& v:victims) {
            auto* target=v->handle.getEntity();
            auto* actor=target && target->m_pBehavior?static_cast<BehaviorAppBase*>(target->m_pBehavior):nullptr;
            if(v->remaining && (!actor || !target->isAlive() || !player || !player->isAlive() ||
                actor->m_Hp<v->hp || actor->m_Rno0!=PistolNative::DownState ||
                (v->unit && (!target->getSlowRate() || target->getSlowRate()->m_pUnit!=v->unit))))Wake(*v);
            if(v->remaining && advance) {
                if(v->unit) {if(v->remaining==1)Wake(*v);else --v->remaining;}
                else if(actor->m_Rno1==PistolNative::GroundLoop) {
                    auto* slow=target->getSlowRate();
                    if(slow && slow->m_pUnit && slow->m_pUnit->m_Rate>=.5f) {
                        v->original=*slow;slow->cleanup();
                        if(slow->setSlowType(eRateType_Em)) {
                            v->unit=slow->m_pUnit;v->unit->m_Rate=0;
                            GunRuntimeLog::Write("M9 ground sleep target=%p state=%X phase=%u duration=%u",target,actor->m_Rno0,actor->m_Rno1,v->remaining);
                        } else {*slow=v->original;v->original.cleanup();Wake(*v);}
                    }
                } else if(++v->fallTicks>180)Wake(*v);
            }
            if(advance)v->dose.Tick();
        }
        victims.erase(std::remove_if(victims.begin(),victims.end(),[](const auto& v){return !v->remaining && (!v->handle.getEntity() || !v->dose.expiry);}),victims.end());
    }
};
