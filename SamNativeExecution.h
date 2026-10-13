#pragma once
#include <Pl0000.h>
#include <BehaviorAppBase.h>
#include "SamExecutionPolicy.h"
#include "SamTargets.h"
#include <cmath>

// Native Sam acceptance owns target validation, pairing, action transitions,
// camera and damage. Never manufacture a victim clip or kill via an HP write.
namespace SamNativeExecution
{
    inline bool Ready(Pl0000* player, Entity* target, cZandatsuTarget*& descriptor)
    {
        descriptor=nullptr;
        if (!player || !player->m_pEntity || !target || !target->m_pBehavior || !target->isAlive()) return false;
        auto* enemy=static_cast<BehaviorAppBase*>(target->m_pBehavior);
        const auto delta=target->getTransPos()-player->m_pEntity->getTransPos();
        const float distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
        const Entity* executor=enemy->m_QTETargetExecutor.getEntity();
        for (auto* candidate : {&player->field_1370, &player->field_11B0, &player->field_1220})
        {
            // Kind 2 is the native human-execution descriptor, verified at
            // Sam's acceptance branch 0x484FE7. Other descriptors must retain
            // their own execution/Blade Mode/story dispatch.
            if (candidate->m_TargetHandle.getEntity()!=target || candidate->field_58!=2) continue;
            if (SamExecutionPolicy::Eligible(unsigned(target->m_ObjId),enemy->m_Hp,enemy->m_HpMax,
                distance,true,executor && executor!=player->m_pEntity))
            { descriptor=candidate; return true; }
        }
        return false;
    }
    inline bool Start(Pl0000* player, Entity* target)
    {
        cZandatsuTarget* descriptor=nullptr;
        if (!Ready(player,target,descriptor) || !player->isAlive() || player->isInAir() || player->isBladeModeActive()) return false;
        // Reproduce the native accepted kind-2 branch (0x48501E..0x48502E).
        // Bind through the engine's paired-target controller first; it can
        // reject unavailable pairing metadata without changing player action.
        reinterpret_cast<void(__thiscall*)(Pl0000*,Entity*,const float*,int)>(shared::base+0x77B380)(
            player,target,&player->m_LocalMatrix._41,1);
        if (!player->field_12A8 || player->field_1370.m_TargetHandle.getEntity()!=target ||
            player->field_1370.field_58!=2) return false;
        player->setRno(SamExecutionPolicy::Human,0,0,0);
        return true;
    }
    inline bool OwnedHit(Pl0000* player, Entity* target)
    {
        if (!player || !target || !target->m_pBehavior) return false;
        const auto& hits=target->m_pBehavior->m_AttackHits;
        if (!hits.m_vector || hits.m_size>hits.m_capacity || hits.m_size>128) return false;
        for (size_t i=0;i<hits.m_size;++i)
            if (hits.m_vector[i].m_HitData.field_18.getEntity()==player->m_pEntity) return true;
        return false;
    }
}
