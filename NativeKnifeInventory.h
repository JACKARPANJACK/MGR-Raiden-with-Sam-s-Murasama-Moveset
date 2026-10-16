#pragma once
#include <cItemPossessionBase.h>
#include <PlayerManagerImplement.h>
#include <array>
#include <cstring>
#include "NativeSmgMenuPolicy.h"
#include "SamPlayerIdentity.h"
#include <cScene.h>

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
    inline bool smgMenuReady=false;
    inline std::array<unsigned char,0x48> smgDefinition{};
    inline bool smgDefinitionReady=false;
    inline std::array<unsigned char,0x48> vectorDefinition{};
    inline bool vectorDefinitionReady=false;
    inline std::array<std::array<unsigned char,0x48>,2> pistolDefinitions{};
    inline std::array<bool,2> pistolDefinitionReady{};
    inline bool SupportedCharacter() { return SamPlayerIdentity::Eligible(g_Scene.m_pPlayer); }

    inline void* SmgDefinition(void* registry,unsigned gun=0)
    {
        if(!raidenScene || !SupportedCharacter() || !smgMenuReady) return nullptr;
        if(gun>=NativeSmgMenuPolicy::GunCount)return nullptr;
        auto& definition=gun>=2?pistolDefinitions[gun-2]:gun==1?vectorDefinition:smgDefinition;
        auto& ready=gun>=2?pistolDefinitionReady[gun-2]:gun==1?vectorDefinitionReady:smgDefinitionReady;
        if(!ready)
        {
            const void* source=originalDefinition(registry,0x3800CB76); // RPG, native Type 0 gun possession.
            if(!source) return nullptr;
            std::memcpy(definition.data(),source,definition.size());
            auto* words=reinterpret_cast<unsigned*>(definition.data());
            words[0]=NativeSmgMenuPolicy::ItemFor(gun);
            words[2]=NativeSmgMenuPolicy::AliasFor(gun);
            // Each firearm owns a distinct native inventory object.
            words[4]=1;
            std::memset(definition.data()+0x18,0,32);
            const char* name=gun>=2?PistolPolicy::Get(gun-2).name:gun==1?"KRISS Vector SMG":"Assault Rifle";
            std::memcpy(definition.data()+0x18,name,std::strlen(name));
            ready=true;
        }
        return definition.data();
    }

    inline void* __fastcall LookupDefinition(void* registry,void*,unsigned alias)
    {
        void* native=originalDefinition(registry,alias);
        if(!native)for(unsigned gun=2;gun<NativeSmgMenuPolicy::GunCount;++gun)
            if(alias==NativeSmgMenuPolicy::AliasFor(gun))return SmgDefinition(registry,gun);
        if(!native && alias==NativeSmgMenuPolicy::VectorAlias) return SmgDefinition(registry,true);
        if(!native && alias==NativeSmgMenuPolicy::Alias) return SmgDefinition(registry);
        if (native || alias!=Alias || !raidenScene || !SupportedCharacter()) return native;
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
        if(!native)for(unsigned gun=2;gun<NativeSmgMenuPolicy::GunCount;++gun)
            if(id==NativeSmgMenuPolicy::ItemFor(gun))return SmgDefinition(registry,gun);
        if(!native && id==NativeSmgMenuPolicy::VectorItemId) return SmgDefinition(registry,true);
        if(!native && id==NativeSmgMenuPolicy::ItemId) return SmgDefinition(registry);
        return native || id!=44 || !raidenScene || !SupportedCharacter() ? native : LookupDefinition(registry,nullptr,Alias);
    }
    inline unsigned __cdecl EquippedAlias()
    {
        if (!SupportedCharacter()) return originalEquippedAlias();
        if(raidenScene && smgMenuReady && g_pPlayerManager &&
            NativeSmgMenuPolicy::GunSlot(g_pPlayerManager->getSubWeaponEquipped()))
            return NativeSmgMenuPolicy::AliasFor(NativeSmgMenuPolicy::IndexFor(g_pPlayerManager->getSubWeaponEquipped()));
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
        if (!raidenScene || !SupportedCharacter() || !originalDefinition) return false;
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
    inline cItemPossessionBase* SmgItem(unsigned gun=0)
    {
        return reinterpret_cast<cItemPossessionBase*(__thiscall*)(void*,unsigned)>(shared::base+0x54E5E0)
            (reinterpret_cast<void*>(shared::base+0x1486EA0),NativeSmgMenuPolicy::AliasFor(gun));
    }
    inline bool EnsureFirearm(unsigned gun)
    {
        if(!raidenScene || !SupportedCharacter() || !smgMenuReady || !originalDefinition) return false;
        if(auto* item=SmgItem(gun)) {if(!item->hasPossession()) item->set(1);return true;}
        void* source=SmgDefinition(reinterpret_cast<void*>(shared::base+0x1486A60),gun);
        if(!source) return false;
        auto* item=reinterpret_cast<cItemPossessionBase*(__cdecl*)(void*)>(shared::base+0x551C80)(source);
        if(!item) return false;
        void* manager=reinterpret_cast<void*>(shared::base+0x1486EA0);
        auto** table=*reinterpret_cast<void***>(manager);
        reinterpret_cast<void(__thiscall*)(void*,cItemPossessionBase**)>(table[2])(manager,&item);
        item->set(1);
        return SmgItem(gun)==item;
    }
    inline bool EnsureSmg() {bool ready=true;for(unsigned gun=0;gun<NativeSmgMenuPolicy::GunCount;++gun)ready=EnsureFirearm(gun) && ready;return ready;}

}
