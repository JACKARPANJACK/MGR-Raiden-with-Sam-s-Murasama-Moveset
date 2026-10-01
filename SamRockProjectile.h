#pragma once
#include <Pl0000.h>
#include <CollisionAttackData.h>
#include <Entity.h>
#include <cmath>
#include <cstring>

namespace SamRockProjectile
{
    // Native boss release 0x2A8DB -> 0x206E0 constructs this bullet descriptor.
    // Use the generic bullet factory, never call Em0020 methods on Pl0000.
    inline bool Spawn(Pl0000* player)
    {
        if (!player || !player->m_pEntity || !player->isAlive()) return false;
        // Slot 0x130 takes a pointer to the attack-number descriptor. The SDK
        // labels this argument int, but native 46BC60 dereferences its first word.
        const uint16_t attackNumber = 26;
        auto table = *reinterpret_cast<uintptr_t**>(player);
        using AttackInfo = CollisionAttackData*(__thiscall*)(Pl0000*, const uint16_t*);
        CollisionAttackData* attack = reinterpret_cast<AttackInfo>(table[0x130 / 4])(player, &attackNumber);
        if (!attack) return false;
        alignas(16) unsigned char descriptor[0x320]{};
        using Init = void(__thiscall*)(void*);
        reinterpret_cast<Init>(shared::base + 0x105D0)(descriptor + 0x10);
        reinterpret_cast<Init>(shared::base + 0x10710)(descriptor + 0x1A0);
        reinterpret_cast<Init>(shared::base + 0x1CF30)(descriptor);
        static_assert(sizeof(CollisionAttackData::HitData) == 0x100, "Native hit descriptor size");
        std::memcpy(descriptor + 0x10, &attack->m_CollisionDataContainer, 0x100);
        // Native getAttackInfo allocates a temporary CollisionAttackData.
        auto attackTable = *reinterpret_cast<uintptr_t**>(attack);
        reinterpret_cast<void(__thiscall*)(CollisionAttackData*, unsigned)>(attackTable[1])(attack, 1);
        *reinterpret_cast<unsigned*>(descriptor + 4) = 0x3C001;
        *reinterpret_cast<unsigned*>(descriptor + 0x114) = 0x2B;
        *reinterpret_cast<Entity**>(descriptor + 0x24) = player->m_pEntity;
        *reinterpret_cast<EntityHandle*>(descriptor + 0x28) = player->m_pEntity;

        const auto& pos = player->m_pEntity->getTransPos();
        const float yaw = player->m_Rot.y;
        const float dx = std::sin(yaw), dz = std::cos(yaw);
        Hw::cVec4 origin(pos.x + dx * 2.0f, pos.y + 0.8f, pos.z + dz * 2.0f, 1.0f);
        Hw::cVec4 target(origin.x + dx * 25.0f, origin.y + 1.0f, origin.z + dz * 25.0f, 1.0f);
        Hw::cVec4 rotation(0.0f, yaw, 0.0f, 0.0f);
        using Trajectory = void(__thiscall*)(void*, Hw::cVec4*, Hw::cVec4*, Hw::cVec4*, float, float);
        reinterpret_cast<Trajectory>(shared::base + 0x16E30)(descriptor, &origin, &target, &rotation, 2.0f, 120.0f);
        using Create = Behavior*(__cdecl*)(Entity*, void*);
        Behavior* rock = reinterpret_cast<Create>(shared::base + 0x6D3BE0)(player->m_pEntity, descriptor);
        if (!rock || !rock->m_pEntity || rock->m_pEntity->m_ObjId != static_cast<eObjID>(0x3C001)) return false;
        reinterpret_cast<Init>(shared::base + 0x202820)(rock);
        Hw::cVec4 velocity(dx * 0.5f, 0.15f, dz * 0.5f, 0.0f);
        reinterpret_cast<void(__thiscall*)(Behavior*, Hw::cVec4*)>(shared::base + 0x201C30)(rock, &velocity);
        return true;
    }
}
