#pragma once
#include "NativeKnifeInventory.h"
#include "gui.h"
#include "NativeSmgMessages.h"
#include <cstdint>

// Win32 retail menu: eleven inventory cells, eight visible carousel rows.
// Extend dispatch/presentation tables in our storage, never the adjacent game tables.
namespace NativeSmgMenu
{
    using Method=void(__thiscall*)(void*);
    using Draw=void(__thiscall*)(void*,int);
    inline Method originalBuild=nullptr,originalCursor=nullptr;
    inline Draw originalDraw=nullptr;
    inline std::array<uintptr_t,11> applyTable{},changedTable{};
    inline std::array<unsigned,11> sounds{};
    inline std::array<unsigned,33> presentation{};
    inline std::array<uintptr_t,33> descriptions{};
    inline uintptr_t applyContinue=0,changedContinue=0;
    inline int& Field(void* menu,unsigned offset)
    {return *reinterpret_cast<int*>(static_cast<char*>(menu)+offset);}
    inline int* Cells(void* menu,unsigned offset)
    {return reinterpret_cast<int*>(static_cast<char*>(menu)+offset);}

    __declspec(naked) inline void ApplySmg()
    {
        __asm {
            mov esi,11
            jmp dword ptr [applyContinue]
        }
    }
    __declspec(naked) inline void ChangedSmg()
    {
        __asm {
            mov eax,11
            jmp dword ptr [changedContinue]
        }
    }

    inline bool SupportedScene()
    {
        // The selector can initialize before the first custom player tick.
        // Resolve the live player here instead of relying on a stale tick flag.
        auto* player=g_Scene.m_pPlayer;
        NativeKnifeInventory::raidenScene=player && player->m_pEntity && SamNativeRuntime::Get().Owns(player);
        const int type=*reinterpret_cast<int*>(shared::base+0x17EA030);
        return NativeKnifeInventory::raidenScene && type!=4 && type!=5 && type!=7;
    }
    inline void __fastcall Build(void* menu,void*)
    {
        // Acquire before native enumeration so all menu lookups see the real item.
        const bool owned=SupportedScene() && NativeKnifeInventory::EnsureSmg();
        originalBuild(menu);
        if(owned) Field(menu,0x3AC)=NativeSmgMenuPolicy::Insert(Cells(menu,0x3C4),
            Cells(menu,0x438),Cells(menu,0x470),Field(menu,0x3AC));
    }
    inline void __fastcall Cursor(void* menu,void*)
    {
        originalCursor(menu);
        if(SupportedScene() && g_pPlayerManager &&
            g_pPlayerManager->getSubWeaponEquipped()==NativeSmgMenuPolicy::Slot)
        {
            const int index=NativeSmgMenuPolicy::Find(Cells(menu,0x3C4),Field(menu,0x3AC));
            if(index>=0) Field(menu,0x36C)=index;
        }
    }
    inline void __fastcall DrawRows(void* menu,void*,int category)
    {
        // Native localized name images are initialized at menu load time.
        // Retain all ten originals and reuse the RPG icon for the new firearm.
        std::memcpy(presentation.data(),reinterpret_cast<void*>(shared::base+0x14B5778),120);
        std::memcpy(presentation.data()+30,presentation.data(),12);
        presentation[30]=reinterpret_cast<unsigned>(NativeSmgMessages::NameKey);
        originalDraw(menu,category);
        if(category!=1) return;
        const int count=Field(menu,0x3AC),cursor=Field(menu,0x36C);
        if(count<1 || count>NativeSmgMenuPolicy::Capacity || cursor<0 || cursor>=count) return;
        void* parts=static_cast<char*>(menu)+0x2FC;
        auto visible=reinterpret_cast<void(__thiscall*)(void*,unsigned,int)>(shared::base+0x8B2310);
        auto text=reinterpret_cast<void(__thiscall*)(void*,unsigned,const char*)>(shared::base+0x8CE090);
        gui::SmgView state{};gui::GetSmgView(state);
        char ammo[32];std::snprintf(ammo,sizeof(ammo),"%u / 30",state.rounds);
        for(int row=0;row<8;++row)
        {
            const int index=NativeSmgMenuPolicy::RowIndex(cursor,count,row);
            if(Cells(menu,0x3C4)[index]!=NativeSmgMenuPolicy::Entry) continue;
            // Label images are type 1; the ammo layouts are native type 4 text.
            // Hide the borrowed RPG name and put the SMG caption in a text layout.
            visible(parts,Field(menu,0x150+row*4),0);
            visible(parts,Field(menu,0x130+row*4),1);
            text(parts,Field(menu,0x130+row*4),"Kriss Vector SMG");
            visible(parts,Field(menu,0x170+row*4),row>=4 && row<=6);
            text(parts,Field(menu,0x170+row*4),ammo);
        }
    }
    inline bool Bytes(unsigned rva,const char* expected,size_t count)
    {return std::memcmp(reinterpret_cast<void*>(shared::base+rva),expected,count)==0;}
    inline bool Install()
    {
        // Check the complete menu contract before changing any native code.
        if(!Bytes(0x5930A0,"\x53\x55\x56\x8B\xF1",5) ||
           !Bytes(0x5928A0,"\x56\x57\x8B\x3D",4) ||
           !Bytes(0x5A48F0,"\x81\xEC\xD0\0\0\0",6) ||
           !Bytes(0x592B0E,"\x83\xF8\x09\x77\x4D\xFF\x24\x85",8) ||
           !Bytes(0x5B43EE,"\x83\xF8\x09\x77\x4A\xFF\x24\x85",8) ||
           !Bytes(0x5A49A0,"\xC7\x44\x24\x40",4) ||
           !Bytes(0x593A94,"\x8B\x04\x8D",3) ||
           !Bytes(0x5A4786,"\x8D\x1C\x9D",3)) return false;
        if(*reinterpret_cast<uintptr_t*>(shared::base+0x592B16)!=shared::base+0x592CE8 ||
           *reinterpret_cast<uintptr_t*>(shared::base+0x5B43F6)!=shared::base+0x5B4654 ||
           *reinterpret_cast<uintptr_t*>(shared::base+0x5A49A4)!=shared::base+0x14B5778 ||
           *reinterpret_cast<uintptr_t*>(shared::base+0x593A97)!=shared::base+0x12567E4 ||
           *reinterpret_cast<uintptr_t*>(shared::base+0x5A4789)!=shared::base+0x12B5470) return false;
        if(!NativeSmgMessages::Install()) return false;
        std::memcpy(descriptions.data(),reinterpret_cast<void*>(shared::base+0x12B5470),120);
        descriptions[30]=reinterpret_cast<uintptr_t>(NativeSmgMessages::TypeKey);
        descriptions[31]=reinterpret_cast<uintptr_t>(NativeSmgMessages::DescriptionKey);
        descriptions[32]=0;
        std::memcpy(applyTable.data(),reinterpret_cast<void*>(shared::base+0x592CE8),40);
        std::memcpy(changedTable.data(),reinterpret_cast<void*>(shared::base+0x5B4654),40);
        std::memcpy(sounds.data(),reinterpret_cast<void*>(shared::base+0x12567E4),40);
        applyContinue=shared::base+0x592B62;changedContinue=shared::base+0x5B443F;
        applyTable[10]=reinterpret_cast<uintptr_t>(ApplySmg);
        changedTable[10]=reinterpret_cast<uintptr_t>(ChangedSmg);
        sounds[10]=sounds[0];
        static SafeHook::Hook build(reinterpret_cast<void*>(shared::base+0x5930A0),
            reinterpret_cast<void*>(Build),true,reinterpret_cast<void**>(&originalBuild));
        static SafeHook::Hook cursor(reinterpret_cast<void*>(shared::base+0x5928A0),
            reinterpret_cast<void*>(Cursor),true,reinterpret_cast<void**>(&originalCursor));
        static SafeHook::Hook draw(reinterpret_cast<void*>(shared::base+0x5A48F0),
            reinterpret_cast<void*>(DrawRows),true,reinterpret_cast<void**>(&originalDraw));
        injector::WriteMemory<unsigned char>(shared::base+0x592B10,10,true);
        injector::WriteMemory<unsigned char>(shared::base+0x5B43F0,10,true);
        injector::WriteMemory<uintptr_t>(shared::base+0x592B16,reinterpret_cast<uintptr_t>(applyTable.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5B43F6,reinterpret_cast<uintptr_t>(changedTable.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A49A4,reinterpret_cast<uintptr_t>(presentation.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x593A97,reinterpret_cast<uintptr_t>(sounds.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A4789,reinterpret_cast<uintptr_t>(descriptions.data()),true);
        NativeKnifeInventory::smgMenuReady=true;
        return true;
    }
}
