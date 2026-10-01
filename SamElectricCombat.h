#pragma once
#include <Pl0000.h>
#include <BehaviorAppBase.h>
#include <cSlowRateManager.h>
#include <memory>
#include <vector>
#include <random>
#include <cmath>
#include "SamElectricPolicy.h"
#include "SamBalancePolicy.h"
#include "SamTargets.h"
#include "SamVisualEffects.h"
#include <CharacterControl.h>

// Electrified strikes use engine hit ownership, not a random roll each frame.
// Stun is a bounded mod effect; native boss/QTE controllers remain untouched.
class SamElectricCombat
{
    struct Victim
    {
        EntityHandle handle;
        cSlowRateUnit* unit = nullptr;
        cSlowRate originalSlow;
        float originalRate = 1.0f;
        unsigned remaining = 0, cooldown = 0, visualTicks = 0;
        int hp = -1;
        bool grapple = false;
        cEspControler visual;
    };
    std::vector<std::unique_ptr<Victim>> victims;
    std::mt19937 random{std::random_device{}()};
    unsigned pulseTicks = 0;

    static bool Ordinary(Entity* entity)
    {
        if (!entity || !entity->m_pBehavior || !entity->isAlive()) return false;
        return SamElectricPolicy::Ordinary(unsigned(entity->m_ObjId));
    }
    static float Distance(Entity* a, Entity* b)
    {
        const auto& x = a->getTransPos(); const auto& y = b->getTransPos();
        float dx=x.x-y.x, dy=x.y-y.y, dz=x.z-y.z;
        return std::sqrt(dx*dx+dy*dy+dz*dz);
    }
    Victim& Track(Entity* target)
    {
        for (auto& v : victims) if (v->handle.getEntity() == target) return *v;
        auto v = std::make_unique<Victim>(); v->handle = target;
        victims.push_back(std::move(v)); return *victims.back();
    }
    static void Release(Victim& v)
    {
        Entity* entity = v.handle.getEntity();
        if (entity && v.unit && entity->getSlowRate()->m_pUnit == v.unit)
            *entity->getSlowRate() = v.originalSlow;
        v.originalSlow.cleanup();
        v.unit = nullptr; v.remaining = 0; v.grapple = false;
    }
    static bool Hold(Victim& v, unsigned ticks, bool grapple)
    {
        Entity* target = v.handle.getEntity();
        if (!Ordinary(target)) return false;
        auto* slow = target->getSlowRate();
        if (!slow || !slow->m_pUnit || slow->m_pUnit->m_Rate < 0.5f || v.remaining) return false;
        // Detach a managed copy before allocating an actor-local slow unit.
        // A shared engine unit must not make all enemies freeze together.
        v.originalRate = slow->m_pUnit->m_Rate;
        v.originalSlow = *slow;
        slow->cleanup();
        if (!slow->setSlowType(eRateType_Em))
        { *slow = v.originalSlow; v.originalSlow.cleanup(); return false; }
        v.unit = slow->m_pUnit;
        v.unit->m_Rate = 0.08f; v.remaining = ticks; v.cooldown = 180;
        v.grapple = grapple; return true;
    }
    static bool OwnedHit(Pl0000* player, Behavior* target)
    {
        const auto& hits = target->m_AttackHits;
        if (!hits.m_vector || hits.m_size > hits.m_capacity || hits.m_size > 128) return false;
        for (size_t i=0; i<hits.m_size; ++i)
            if (hits.m_vector[i].m_HitData.field_18.getEntity() == player->m_pEntity) return true;
        return false;
    }
public:
    // Build the same native effect descriptor as an EffectTrack, with an
    // explicit resource bank. This works on enemies without casting to Pl0000.
    static void Lightning(Pl0000* player, Entity* anchor, cEspControler* controller)
    {
        if (!player || !anchor || !anchor->m_pBehavior) return;
        // Core 316 is an installed electric/plasma impact particle used by
        // native collision effects. 0x7C0000 selects the core effect bank.
        SamVisualEffects::Spawn(anchor->m_pBehavior,anchor,0x7C0000,316,controller);
    }
    void Reset()
    {
        for (auto& v : victims)
        {
            Release(*v);
            if (v->visualTicks) v->visual.FadeUnits(3.0f, 0.0f);
            v->visualTicks = v->cooldown = 0; v->hp = -1;
        }
        // Keep controllers alive while native particles finish fading.
        pulseTicks = 0;
    }
    void Stun(Pl0000* player, Entity* target, unsigned ticks = 90)
    {
        if (!Ordinary(target)) return;
        auto& victim = Track(target);
        if (Hold(victim,ticks,false))
        { Lightning(player,target,&victim.visual); victim.visualTicks=35; }
    }
    bool FinisherAvailable(Pl0000* player)
    {
        Entity* target = SamTargets::Find(player);
        if (!Ordinary(target) || Distance(player->m_pEntity,target) > 3.0f) return false;
        auto* enemy = static_cast<BehaviorAppBase*>(target->m_pBehavior);
        return SamElectricPolicy::Finisher(enemy->m_Hp,enemy->m_HpMax,Distance(player->m_pEntity,target));
    }
    void Grab(Pl0000* player)
    {
        Entity* target = SamTargets::Find(player,2.5f,true);
        if (Ordinary(target) && Distance(player->m_pEntity,target) <= 2.5f)
            Hold(Track(target), 180, true);
    }
    bool Slam(Pl0000* player)
    {
        for (auto& v : victims) if (v->grapple && v->remaining)
        {
            Entity* target = v->handle.getEntity();
            if (!Ordinary(target) || Distance(player->m_pEntity,target) > 3.0f) { Release(*v); return false; }
            auto* enemy = static_cast<BehaviorAppBase*>(target->m_pBehavior);
            int native = reinterpret_cast<int(__thiscall*)(Pl0000*,int)>(shared::base + 0x77ED30)(player,12);
            enemy->damage(SamBalancePolicy::Damage(native,12,false,1,enemy->m_HpMax),false);
            Lightning(player,target,&v->visual); v->visualTicks = 24;
            Release(*v);
            return true;
        }
        return false;
    }
    void Strike(Pl0000* player, bool damage = false)
    {
        Entity* target = SamTargets::Find(player);
        if (target && target->m_pBehavior && target->isAlive() && Distance(player->m_pEntity,target) <= 12.0f)
        {
            auto& v = Track(target);
            Lightning(player,target,&v.visual); v.visualTicks=35;
            if (damage && SamTargets::Enemy(target))
            {
                auto* enemy = static_cast<BehaviorAppBase*>(target->m_pBehavior);
                int native = reinterpret_cast<int(__thiscall*)(Pl0000*,int)>(shared::base + 0x77ED30)(player,12);
                enemy->damage(SamBalancePolicy::Damage(native,12,false,1,enemy->m_HpMax),false);
            }
        }
    }
    void EndGrab()
    {
        for (auto& v : victims) if (v->grapple) Release(*v);
    }
    void Tick(Pl0000* player, bool regularAttack, bool heavy, bool lightningUltimate, bool grabActive)
    {
        for (auto& v : victims)
        {
            if (v->cooldown) --v->cooldown;
            if (v->visualTicks && --v->visualTicks == 0) v->visual.FadeUnits(3.0f,0.0f);
            Entity* target = v->handle.getEntity();
            if (v->remaining && (!target || !target->isAlive() || !player->isAlive() ||
                player->isBladeModeActive() || Trigger::StaFlags.STA_QTE ||
                (v->grapple && !grabActive) || --v->remaining == 0)) Release(*v);
            if (v->remaining && v->grapple && target)
            {
                auto anchor = player->m_pEntity->getTransPos();
                auto pos = target->getTransPos();
                anchor.x += std::sin(player->m_Rot.y)*1.2f;
                anchor.z += std::cos(player->m_Rot.y)*1.2f;
                pos.x += (anchor.x-pos.x)*0.25f; pos.z += (anchor.z-pos.z)*0.25f;
                target->m_pBehavior->setTransPos(pos);
                if (target->m_pBehavior->m_pCharacterControl)
                    target->m_pBehavior->m_pCharacterControl->setPosition(pos,TRUE);
                target->setTransPos(pos);
            }
        }
        Entity* target = SamTargets::Find(player);
        if (!target || !target->m_pBehavior || !target->isAlive()) return;
        // Summon real engine particles on the struck/selected enemy as well as
        // Raiden's body, without adding a second damage pass to the sequence.
        if (lightningUltimate && Distance(player->m_pEntity,target) <= 12.0f)
        {
            auto& v = Track(target);
            if (++pulseTicks >= 30 && !v.visualTicks)
            { pulseTicks=0; Lightning(player,target,&v.visual); v.visualTicks=18; }
        }
        else pulseTicks=0;
        if (!Ordinary(target)) return;
        auto& v = Track(target);
        int hp = static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_Hp;
        bool hit = SamElectricPolicy::ConfirmedHit(v.hp,hp,regularAttack,
            OwnedHit(player,target->m_pBehavior),Distance(player->m_pEntity,target));
        v.hp = hp;
        if (hit && SamElectricPolicy::Proc(std::uniform_int_distribution<unsigned>(0,99)(random),heavy,v.cooldown))
        {
            if (Hold(v,45,false))
            { Lightning(player,target,&v.visual); v.visualTicks=35; }
        }
    }
};
