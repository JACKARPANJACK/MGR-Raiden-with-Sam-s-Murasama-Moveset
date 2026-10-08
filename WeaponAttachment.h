#pragma once
#include <Pl0000.h>

namespace WeaponAttachment
{
    // Raiden's custom weapon placement: choose the native hand marker,
    // attach in slot 5, then publish the handle and cached marker together.
    inline bool Bind(Pl0000* player, Entity* entity, int handMarker = -1, int gripBone = -1)
    {
        if (!player || !player->m_pEntity || !entity || !entity->m_pBehavior) return false;
        reinterpret_cast<void(__thiscall*)(Pl0000*)>(shared::base + 0x78AAE0)(player);
        if (handMarker >= 0) player->m_CustomWeaponBone = handMarker;
        player->removeConstraint(5);
        entity->m_pBehavior->m_pOwner = player;
        player->attachObject(5, player->m_pEntity, entity, player->m_CustomWeaponBone, gripBone);
        auto* mount = player->getConstraintById(5);
        if (!mount || mount->m_MainEntity.getEntity() != player->m_pEntity ||
            mount->m_ObjectEntityHandle.getEntity() != entity ||
            mount->m_nBone != static_cast<unsigned>(player->m_CustomWeaponBone) ||
            mount->m_nRotationBone != static_cast<unsigned>(gripBone)) return false;
        player->m_CustomWeaponHandle = entity;
        player->field_1420 = player->m_CustomWeaponBone;
        entity->m_pBehavior->onDisp();
        return true;
    }
}
