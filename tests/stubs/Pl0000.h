#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>
namespace lib {
template<class T> struct AllocatedArray { T* m_pArray = nullptr; int m_Size = 0; int m_Capacity = 0; };
}
struct AnimationMap {
    struct Unit { int m_Id = 0; int field_4 = 0; char m_pName[4]{}; int cancel = 0; };
    lib::AllocatedArray<Unit>* m_pUnits;
};
struct Slot { AnimationMap::Unit* m_pAnimationMap = nullptr; };
struct Entity;
struct Pl0000 {
    Entity* m_pEntity;
    AnimationMap* m_pAnimationMap;
    lib::AllocatedArray<Slot>* m_pAnimationSlot = nullptr;
};
struct Archive {
    std::unordered_set<std::string> files;
    void* getFileNameData(const char* name) { return files.count(name) ? this : nullptr; }
};
struct Entity {
    unsigned m_ObjId = 0x10010;
    Pl0000* m_pBehavior = nullptr;
    Archive m_EntityData;
    bool alive = true;
};
struct EntityHandle {
    Entity* entity = nullptr;
    Entity* getEntity() const { return entity && entity->alive ? entity : nullptr; }
    EntityHandle& operator=(Entity* value) { entity = value; return *this; }
};
