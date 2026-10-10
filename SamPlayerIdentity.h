#pragma once
#include <Behavior.h>
#include "SamPlayerPolicy.h"
namespace SamPlayerIdentity {
inline bool Eligible(const cObj* player) {
    return player && SamPlayerPolicy::Eligible(unsigned(player->m_ObjId),
        player->m_pEntity ? unsigned(player->m_pEntity->m_ObjId) : 0, player->m_pEntity != nullptr);
}
}
