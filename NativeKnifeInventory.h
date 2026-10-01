#pragma once
#include <cItemPossessionBase.h>
#include <PlayerManagerImplement.h>
#include <array>
#include <cstring>

// Native DLC3_BladeKnife: itemlist.bxm ID 44, Type 14, capacity 10.
// Raiden's menu already enumerates this alias and maps it to subweapon 10.
// Supply its definition in base-game scenes and let the engine own the item.
namespace NativeKnifeInventory
{
    constexpr unsigned Alias=0x154B4AAB, Slot=10, Capacity=10;
    using DefinitionLookup=void*(__thiscall*)(void*,unsigned);
    inline DefinitionLookup originalDefinition=nullptr;
    inline DefinitionLookup originalDefinitionById=nullptr;
    inline unsigned(__cdecl* originalEquippedAlias)()=nullptr;
    inline std::array<unsigned char,0x48> definition{};
    inline bool definitionReady=false;
    inline bool raidenScene=false;

    inline void* __fastcall LookupDefinition(void* registry,void*,unsigned alias)
    {
        void* native=originalDefinition(registry,alias);
        if (native || alias!=Alias || !raidenScene) return native;
        if (!definitionReady)
        {
            const void* grenade=originalDefinition(registry,0x4B20F7AE);
            if (!grenade) return nullptr;
            std::memcpy(definition.data(),grenade,definition.size());
            auto* words=reinterpret_cast<unsigned*>(definition.data());
            words[0]=44; words[1]=14; words[2]=Alias;
            words[3]=reinterpret_cast<unsigned(__cdecl*)(const char*)>(shared::base+0x5FDE60)("it0640");
            if (words[3]==0xFFFFFFFF) return nullptr;
            words[4]=Capacity; words[5]=100;
            std::memset(definition.data()+0x18,0,32);
            std::memcpy(definition.data()+0x18,"DLC3_BladeKnife",14);
            words[14]=1; words[15]=words[16]=words[17]=0;
            definitionReady=true;
        }
        return definition.data();
    }
    inline void* __fastcall LookupDefinitionById(void* registry,void*,unsigned id)
    {
        void* native=originalDefinitionById(registry,id);
        return native || id!=44 || !raidenScene ? native : LookupDefinition(registry,nullptr,Alias);
    }
    inline unsigned __cdecl EquippedAlias()
    {
        if (raidenScene && g_pPlayerManager && g_pPlayerManager->getSubWeaponEquipped()==Slot)
            return Alias;
        return originalEquippedAlias();
    }
    inline cItemPossessionBase* Item()
    {
        return reinterpret_cast<cItemPossessionBase*(__thiscall*)(void*,unsigned)>(shared::base+0x54E5E0)
            (reinterpret_cast<void*>(shared::base+0x1486EA0),Alias);
    }
    inline bool Ensure()
    {
        if (!raidenScene || !originalDefinition) return false;
        if (Item()) return true;
        void* source=LookupDefinition(reinterpret_cast<void*>(shared::base+0x1486A60),nullptr,Alias);
        if (!source) return false;
        auto* item=reinterpret_cast<cItemPossessionBase*(__cdecl*)(void*)>(shared::base+0x551C80)(source);
        if (!item) return false;
        // Native cItemPossessionManager::add takes a pointer-to-item, vtable 2.
        void* manager=reinterpret_cast<void*>(shared::base+0x1486EA0);
        auto** table=*reinterpret_cast<void***>(manager);
        reinterpret_cast<void(__thiscall*)(void*,cItemPossessionBase**)>(table[2])(manager,&item);
        item->set(Capacity);
        return Item()==item;
    }
}
