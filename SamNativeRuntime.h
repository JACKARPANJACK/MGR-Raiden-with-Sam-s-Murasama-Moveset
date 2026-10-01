#pragma once
#include <Pl1400.h>
#include <AnimationMapManagerImplement.h>
#include <unordered_map>
#include <memory>
#include <cstring>
#include "SamResourceManager.h"
#include "SamTogglePolicy.h"
#include "SamUltimatePolicy.h"
#include "SamBalancePolicy.h"
#include "SamElectricPolicy.h"
#include "SamTargets.h"
#include "SamVisualEffects.h"
#include <BehaviorAppBase.h>

// A record belongs to an engine allocation, not to whichever player pointer
// happens to be in g_Scene this frame. Records survive until native shutdown.
class SamNativeRuntime
{
    struct Record
    {
        uintptr_t table[0x428 / 4]{};
        uintptr_t* original = nullptr;
        AnimationMap* map = nullptr;
        bool active = false;
        BattleParameterImplement* raidenParameters = nullptr;
        std::unique_ptr<BattleParameterImplement> samParameters;
        float runSpeed = 1.0f;
        bool addonDamage = false;
        unsigned hitBoxes = 1;
        uintptr_t raidenStates[3]{};
        uintptr_t samStates[3]{};
    };
    std::unordered_map<Pl0000*, std::unique_ptr<Record>> records;
    AnimationMap* samMap = nullptr;
    bool allocationReady = false;
    static void __fastcall CombatAction(Pl0000* player, void*)
    {
        if (Get().Active(player) && player->m_Rno0 == SamUltimatePolicy::Action)
        {
            // Shared animation motion update used by the reference custom-action
            // handler. Physics/lifecycle and damage dispatch remain engine-owned.
            reinterpret_cast<void(__thiscall*)(Pl0000*, float, float)>(shared::base + 0x794790)(player, 1.0f, 1.0f);
            return;
        }
        reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base + 0x49C910)(player);
    }

    static void NativeEffect(Pl0000* player, int number, cEspControler* controller, uintptr_t callback)
    {
        number = SamVisualEffects::ChargeNumber(number);
        const int bank = SamVisualEffects::Bank(number);
        // Keep the native descriptor, both weapon parents and the distinct
        // owner/control pointer semantics of 3DC and 3E0. Model bank is scoped
        // to this synchronous call; it also supplies correct effect metadata.
        const auto originalBank = player->m_ModelIndex;
        __try
        {
            if (bank != -1) player->m_ModelIndex = static_cast<decltype(player->m_ModelIndex)>(bank);
            reinterpret_cast<void(__thiscall*)(Pl0000*,int,cEspControler*)>(shared::base + callback)(player,number,controller);
        }
        __finally { player->m_ModelIndex = originalBank; }
    }
    static void __fastcall PlayerEffect(Pl0000* player, void*, int number, cEspControler* controller)
    {
        NativeEffect(player,number,controller,0x6A5600);
    }
    static void __fastcall PlayerEffectOwner(Pl0000* player, void*, int number, cEspControler* controller)
    {
        NativeEffect(player,number,controller,0x6A5570);
    }
    static CollisionAttackData* __fastcall AttackInfo(Pl0000* player, void*, const uint16_t* number)
    {
        auto* attack = reinterpret_cast<CollisionAttackData*(__thiscall*)(Pl0000*, const uint16_t*)>(
            shared::base + 0x46BC60)(player,number);
        if (!attack || !number || !Get().Active(player) || player->isBladeModeActive() ||
            (!SamUltimatePolicy::Attack(player->m_Rno0) && player->m_Rno0 != SamUltimatePolicy::Action)) return attack;
        const auto it = Get().records.find(player);
        if (it == Get().records.end()) return attack;
        auto& record = *it->second;
        int maxHp = 0;
        Entity* target = SamTargets::Find(player);
        if (target && target->m_pBehavior && SamElectricPolicy::Ordinary(unsigned(target->m_ObjId)))
            maxHp = static_cast<BehaviorAppBase*>(target->m_pBehavior)->m_HpMax;
        auto& hit = attack->m_CollisionDataContainer;
        hit.field_4 = SamBalancePolicy::Damage(hit.field_4,*number,record.addonDamage,record.hitBoxes,maxHp);
        return attack;
    }
    static uintptr_t* States(Pl0000* player)
    {
        return reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(player) + 0x7CC);
    }
    static void ClearStates(uintptr_t* states)
    {
        if (states[0] && states[1])
            reinterpret_cast<void(__thiscall*)(void*, void*)>(shared::base + 0x982DE0)(
                reinterpret_cast<void*>(states[0]), reinterpret_cast<void*>(states[1]));
    }
    static void DestroyStates(uintptr_t* states)
    {
        ClearStates(states);
        if (states[1])
        {
            auto table = *reinterpret_cast<uintptr_t**>(states[1]);
            reinterpret_cast<void(__thiscall*)(void*, unsigned)>(table[1])(reinterpret_cast<void*>(states[1]), 1);
        }
        if (states[2])
        {
            auto table = *reinterpret_cast<uintptr_t**>(states[2]);
            reinterpret_cast<void(__thiscall*)(void*, unsigned)>(table[1])(reinterpret_cast<void*>(states[2]), 1);
        }
        if (states[0])
        {
            void* graph = reinterpret_cast<void*>(states[0]);
            void* impl = *reinterpret_cast<void**>(graph);
            if (impl) reinterpret_cast<void(__thiscall*)(void*)>(shared::base + 0x982620)(impl);
            reinterpret_cast<void(__thiscall*)(void*)>(shared::base + 0x982650)(graph);
            reinterpret_cast<void(__cdecl*)(void*)>(shared::base + 0x9D4920)(graph);
        }
        std::memset(states, 0, 3 * sizeof(uintptr_t));
    }

public:
    static SamNativeRuntime& Get() { static SamNativeRuntime runtime; return runtime; }

    // Run before the player factory can allocate anything. Validate both
    // instructions before writing either allocation or zero-initialization size.
    bool PrepareAllocation()
    {
        auto* a = reinterpret_cast<unsigned char*>(shared::base + 0x6C334B);
        auto* b = reinterpret_cast<unsigned char*>(shared::base + 0x6C335D);
        if (*a == 0x68 && *b == 0x68 &&
            *reinterpret_cast<uint32_t*>(a + 1) == sizeof(Pl1400) &&
            *reinterpret_cast<uint32_t*>(b + 1) == sizeof(Pl1400))
        {
            allocationReady = true;
            return true;
        }
        if (*a != 0x68 || *b != 0x68 ||
            *reinterpret_cast<uint32_t*>(a + 1) != sizeof(Pl0000) ||
            *reinterpret_cast<uint32_t*>(b + 1) != sizeof(Pl0000)) return false;
        DWORD protection;
        if (!VirtualProtect(a, b + 5 - a, PAGE_EXECUTE_READWRITE, &protection)) return false;
        *reinterpret_cast<uint32_t*>(a + 1) = sizeof(Pl1400);
        *reinterpret_cast<uint32_t*>(b + 1) = sizeof(Pl1400);
        DWORD unused;
        VirtualProtect(a, b + 5 - a, protection, &unused);
        FlushInstructionCache(GetCurrentProcess(), a, b + 5 - a);
        allocationReady = true;
        return true;
    }

    void Created(Pl0000* player)
    {
        if (allocationReady && player && records.find(player) == records.end())
            records[player] = std::make_unique<Record>();
    }

    bool Owns(Pl0000* player) const { return records.find(player) != records.end(); }
    bool Active(Pl0000* player) const
    {
        const auto it = records.find(player);
        return it != records.end() && it->second->active;
    }

    bool Activate(Pl0000* player)
    {
        if (!player) return false;
        auto it = records.find(player);
        if (it == records.end()) return false;
        if (!SamResourceManager::Instance().IsRuntimeReady()) return false;
        Record& record = *it->second;
        if (record.active) return true;
        if (!SamTogglePolicy::CanActivate(player->m_Rno0, player->isAlive() != FALSE)) return false;
        const bool airborne = player->isInAir() != FALSE;
        if (!samMap)
        {
            auto* maps = AnimationMapManagerImplement::get();
            if (!maps) return false;
            samMap = maps->addReference(static_cast<eObjID>(0x11400), SamResourceManager::Instance().GetFmerge());
        }
        if (!samMap || !samMap->getUnitByAnim(76)) return false;

        record.original = *reinterpret_cast<uintptr_t**>(player);
        record.map = player->m_pAnimationMap;
        auto* sam = reinterpret_cast<uintptr_t*>(shared::base + 0x129EA84);
        // Populate the genuine Sam extension; never read past Raiden's table.
        std::memcpy(record.table, sam, sizeof(record.table));
        std::memcpy(record.table, record.original, 0x3F8);
        constexpr unsigned combatSlots[] = {
            0x048, 0x04C, 0x050, 0x054, // Sam/DLC update loops process Sam state nodes.
            0x004, // Sam nodes require the Pl1400 runtime type descriptor.
            0x130, 0x134, 0x32C, 0x344, 0x34C, 0x354, 0x358, 0x35C,
            0x360, 0x364, 0x368, 0x36C, 0x370, 0x374, 0x378, 0x380,
            0x388, 0x394, 0x39C, 0x3C0, 0x3C4, 0x3C8, 0x3CC,
            0x3D0, 0x3FC, 0x3DC, 0x3E0, 0x3E4, 0x3E8, 0x3EC, 0x3F0
        };
        for (unsigned slot : combatSlots) record.table[slot / 4] = sam[slot / 4];
        // Sam's Blade Mode callbacks include the 0x330 and 0x3A0..3BC
        // helpers; retaining Raiden's versions still invokes Raiden weapon logic.
        std::memcpy(record.table + 0x328/4,sam + 0x328/4,0x100);
        record.table[0x3C8 / 4] = reinterpret_cast<uintptr_t>(&CombatAction);
        record.table[0x130 / 4] = reinterpret_cast<uintptr_t>(&AttackInfo);
        record.table[0x3DC / 4] = reinterpret_cast<uintptr_t>(&PlayerEffectOwner);
        record.table[0x3E0 / 4] = reinterpret_cast<uintptr_t>(&PlayerEffect);
        record.raidenParameters = player->m_pBattleParameterImplement;
        if (!record.samParameters)
        {
            void* bin = SamResourceManager::Instance().GetFmerge()->getFileNameData("pl1400_battleParameter.bin");
            if (!bin || !record.raidenParameters) return false;
            record.samParameters = std::make_unique<BattleParameterImplement>(record.raidenParameters->m_Allocator,bin);
        }
        record.runSpeed = player->m_NinjaRunSpeedRate;
        player->m_NinjaRunSpeedRate = record.runSpeed * SamBalancePolicy::RunSpeed;

        std::memcpy(record.raidenStates, States(player), sizeof(record.raidenStates));
        ClearStates(record.raidenStates);
        auto* actor = reinterpret_cast<Pl1400*>(player);
        std::memset(reinterpret_cast<char*>(actor) + sizeof(Pl0000), 0, sizeof(Pl1400) - sizeof(Pl0000));
        actor->field_5414 = 0.03f;
        actor->field_5418 = 0.30f;
        actor->field_541C = 0.84f;
        actor->field_5424 = 8;
        actor->field_5434 = 4096;
        actor->field_545C = 1;
        player->m_pBattleParameterImplement = record.samParameters.get();
        player->m_pAnimationMap = samMap;
        *reinterpret_cast<uintptr_t**>(player) = record.table;
        // Native Sam initializer constructs the genuine factory and 0x6C0
        // context, including Iai Ready/Hold/Attack and Datsu Jump/Short nodes.
        // Never ask Raiden's factory to interpret Sam's node numbers.
        if (!record.samStates[0])
        {
            reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base + 0x493B60)(player);
            std::memcpy(record.samStates, States(player), sizeof(record.samStates));
        }
        else std::memcpy(States(player), record.samStates, sizeof(record.samStates));
        record.active = true;
        player->setRno(SamTogglePolicy::EntryState(airborne), 0, 0, 0);

        return true;
    }

    void Deactivate(Pl0000* player, bool resetAction = true)
    {
        auto it = records.find(player);
        if (it == records.end() || !it->second->active) return;
        Record& record = *it->second;
        const bool resetOwnedAction = resetAction &&
            (SamTogglePolicy::OwnsAction(player->m_Rno0) || player->isBladeModeActive());
        const bool airborne = resetOwnedAction && player->isInAir() != FALSE;
        DestroyStates(record.samStates);
        std::memcpy(States(player), record.raidenStates, sizeof(record.raidenStates));
        if (*reinterpret_cast<uintptr_t**>(player) == record.table)
            *reinterpret_cast<uintptr_t**>(player) = record.original;
        if (player->m_pAnimationMap == samMap) player->m_pAnimationMap = record.map;
        player->m_pBattleParameterImplement = record.raidenParameters;
        player->m_NinjaRunSpeedRate = record.runSpeed;
        record.addonDamage = false;
        record.active = false;

        if (resetOwnedAction)
        {
            player->setRno(SamTogglePolicy::ExitState(airborne), 0, 0, 0);
            // Replace the active Sam sequence as well as its controller.
            if (airborne)
                player->requestAnimationByName("0232", 0, 0.05f, 1.0f, 0, 0.0f, 1.0f);
            else
                player->requestAnimationByMap(4);
        }
    }

    bool CreatePlayerEffect(Pl0000* player, int number, cEspControler* controller)
    {
        if (!Active(player)) return false;
        PlayerEffect(player,nullptr,number,controller);
        return true;
    }
    void ConfigureDamage(Pl0000* player, bool addon, unsigned boxes = 1)
    {
        auto it = records.find(player);
        if (it != records.end()) { it->second->addonDamage = addon; it->second->hitBoxes = boxes ? boxes : 1; }
    }

    void BeforeShutdown(Pl0000* player)
    {
        Deactivate(player, false);
        auto it = records.find(player);
        if (it != records.end())
        {
            DestroyStates(it->second->samStates);
            it->second->samParameters.reset();
        }
    }
    void Destroyed(Pl0000* player) { records.erase(player); }
};
