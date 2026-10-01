#pragma once
#include <EntitySystem.h>
#include <Pl0000.h>
#include <cmath>
#include "SamElectricPolicy.h"
namespace SamTargets
{
    inline bool Enemy(Entity* target)
    {
        if (!target || !target->m_pBehavior || !target->isAlive()) return false;
        if (SamElectricPolicy::Ordinary(unsigned(target->m_ObjId))) return true;
        switch (unsigned(target->m_ObjId))
        {
        case 0x20020: case 0x2002F: case 0x20030: case 0x20033: case 0x20035:
        case 0x20040: case 0x20060: case 0x20070: case 0x20071: case 0x20080:
        case 0x20081: case 0x20091: case 0x200A0: case 0x200A1: case 0x20100:
        case 0x20110: case 0x20120: case 0x20121: case 0x20130: case 0x20190:
        case 0x201A0: case 0x20310: case 0x20700: return true;
        default: return false;
        }
    }
    inline Entity* Find(Pl0000* player, float range = 12.0f, bool cyborgOnly = false)
    {
        if (!player || !player->m_pEntity) return nullptr;
        const auto pos = player->m_pEntity->getTransPos();
        const float fx = std::sin(player->m_Rot.y), fz = std::cos(player->m_Rot.y);
        Entity* best = nullptr; float score = range*range*2.0f;
        auto& list = g_EntitySystem.getEntityList();
        auto it = list.begin();
        const unsigned count = unsigned((std::max)(0,(std::min)(8192,list.getSize())));
        for (unsigned visited=0; visited<count && it != Hw::cFixedList<Entity*>::iterator{}; ++visited,++it)
        {
            Entity* candidate = *it;
            if (!Enemy(candidate) || (cyborgOnly && !SamElectricPolicy::Ordinary(unsigned(candidate->m_ObjId)))) continue;
            const auto p = candidate->getTransPos();
            float dx=p.x-pos.x,dy=p.y-pos.y,dz=p.z-pos.z;
            float distance=dx*dx+dy*dy+dz*dz;
            if (distance > range*range) continue;
            float front=dx*fx+dz*fz;
            if (front < -0.5f) continue;
            float weighted=distance+(front < 0 ? 2.0f : 0.0f);
            if (weighted < score) {best=candidate;score=weighted;}
        }
        return best;
    }
}
