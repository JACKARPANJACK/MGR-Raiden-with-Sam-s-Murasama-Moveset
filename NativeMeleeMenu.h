#pragma once
#include "NativeMeleeMenuPolicy.h"
#include "NativeSmgMessagePolicy.h"
#include "gui.h"
#include <cstdint>
#include <cstring>

// The stock custom-melee list has four cells at +3F0 immediately before
// the primary-sword list. Relocate every consumer, including native builds,
// rather than appending into the primary list's storage.
namespace NativeMeleeMenu
{
    using namespace NativeMeleeMenuPolicy;
    inline int entries[Capacity]={-1,-1,-1,-1,-1,-1,-1};
    inline bool ready=false;
    inline bool(*supported)()=nullptr;
    inline std::array<unsigned,6> sounds{};
    inline std::array<uintptr_t,18> presentation{},descriptions{};
    inline uintptr_t applyContinue=0,applySkip=0,changedContinue=0,changedSkip=0;
    inline int& Field(void* menu,unsigned offset)
    {return *reinterpret_cast<int*>(static_cast<char*>(menu)+offset);}
    inline int SelectedEntry(void* menu)
    {
        const int cursor=Field(menu,0x374),count=Field(menu,0x3B4);
        return count>0 && count<=Capacity && cursor>=0 && cursor<count?entries[cursor]:-2;
    }
    inline int __cdecl Queue(void* menu)
    {
        if(!supported || !supported())return 0;
        const int target=Selection(SelectedEntry(menu));
        if(target<0)return 0;
        gui::SelectInventoryWeapon(target);
        return 1;
    }
    inline int __cdecl IsChanged(void* menu)
    {
        if(!supported || !supported())return -1;
        return Selection(SelectedEntry(menu))!=gui::SelectedWeapon();
    }
    __declspec(naked) inline void Apply()
    {
        __asm {
            pushad
            push edi
            call Queue
            add esp,4
            test eax,eax
            jz nativeApply
            popad
            jmp dword ptr [applySkip]
        nativeApply:
            popad
            mov eax,dword ptr [edi+374h]
            jmp dword ptr [applyContinue]
        }
    }
    __declspec(naked) inline void Changed()
    {
        __asm {
            pushad
            push esi
            call IsChanged
            add esp,4
            mov dword ptr [esp+28],eax
            popad
            cmp eax,-1
            je nativeChanged
            test eax,eax
            jz unchanged
            mov ebx,ebp
        unchanged:
            jmp dword ptr [changedSkip]
        nativeChanged:
            mov edx,dword ptr [esi+374h]
            jmp dword ptr [changedContinue]
        }
    }
    inline void Build(void* menu)
    {
        if(!ready)return;
        Field(menu,0x3B4)=Insert(entries,Field(menu,0x3B4));
        Field(menu,0x3A4)=0;
    }
    inline void Cursor(void* menu)
    {
        if(!ready)return;
        const int entry=Entry(gui::PendingWeapon()>=0?gui::PendingWeapon():gui::SelectedWeapon());
        const int count=Field(menu,0x3B4);
        if(count<1 || count>Capacity)return;
        for(int i=0;i<count;++i)if(entries[i]==entry){Field(menu,0x374)=i;break;}
    }
    inline void RefreshPresentation()
    {
        if(!ready)return;
        std::memcpy(presentation.data(),reinterpret_cast<void*>(shared::base+0x12B5410),36);
        for(int i=3;i<6;++i) {
            std::memcpy(presentation.data()+i*3,presentation.data(),12);
            constexpr const char* shortNames[]={"HUD_ITEM_NAME_S_0031","HUD_ITEM_NAME_S_0032","HUD_ITEM_NAME_S_0033"};
            presentation[i*3]=reinterpret_cast<uintptr_t>(shortNames[i-3]);
            presentation[i*3+1]=i==3 ? 0x41547db3 : (i==4 ? 0xee93139 : 0x31a52db4);
            presentation[i*3+2]=0;
        }
    }

    struct Site {unsigned rva;const char* bytes;unsigned size;};
    inline constexpr Site Indexed[]={
        {0x592C6F,"\x8B\x84\x87\xF0\x03\0\0",7},
        {0x5933FE,"\x89\x8C\x86\xF0\x03\0\0",7},
        {0x59342A,"\xC7\x84\x86\xF0\x03\0\0\xFF\xFF\xFF\xFF",11},
        {0x593AC6,"\x8B\x8C\x91\xF0\x03\0\0",7},
        {0x593BE6,"\x8B\x84\x86\xF0\x03\0\0",7},
        {0x5A47AF,"\x8B\x84\x86\xF0\x03\0\0",7},
        {0x5B44E3,"\x8B\x84\x96\xF0\x03\0\0",7}
    };
    inline constexpr Site Other[]={
        {0x592A2E,"\x8D\x8E\xF0\x03\0\0",6},
        {0x5A4979,"\x8D\x87\xF0\x03\0\0",6},
        {0x592C69,"\x8B\x87\x74\x03\0\0",6},
        {0x5B44DD,"\x8B\x96\x74\x03\0\0",6}
    };
    inline bool Install(bool(*scene)())
    {
        for(const auto& site:Indexed)if(std::memcmp(reinterpret_cast<void*>(shared::base+site.rva),site.bytes,site.size))return false;
        for(const auto& site:Other)if(std::memcmp(reinterpret_cast<void*>(shared::base+site.rva),site.bytes,site.size))return false;
        const auto pointer=[](unsigned at,unsigned target) {
            return *reinterpret_cast<uintptr_t*>(shared::base+at)==shared::base+target;
        };
        if(!pointer(0x593AD7,0x1256878) || !pointer(0x5A49B4,0x12B5410) ||
           !pointer(0x5A47C1,0x12B544C))return false;
        supported=scene;
        std::memcpy(sounds.data(),reinterpret_cast<void*>(shared::base+0x1256878),12);
        std::memcpy(descriptions.data(),reinterpret_cast<void*>(shared::base+0x12B544C),36);
        for(int i=3;i<6;++i) {
            sounds[i]=sounds[0];
            descriptions[i*3]=reinterpret_cast<uintptr_t>(NativeSmgMessagePolicy::Keys[5+(i-3)*3]);
            descriptions[i*3+1]=reinterpret_cast<uintptr_t>(NativeSmgMessagePolicy::Keys[4+(i-3)*3]);
            descriptions[i*3+2]=0;
        }
        const uintptr_t address=reinterpret_cast<uintptr_t>(entries);
        for(const auto& site:Indexed) {
            injector::WriteMemory<unsigned char>(shared::base+site.rva+1,(site.bytes[1]&0x38)|4,true);
            injector::WriteMemory<unsigned char>(shared::base+site.rva+2,(site.bytes[2]&0xF8)|5,true);
            injector::WriteMemory<uintptr_t>(shared::base+site.rva+3,address,true);
        }
        for(const auto& site:{Other[0],Other[1]}) {
            injector::WriteMemory<unsigned char>(shared::base+site.rva,site.rva==0x592A2E?0xB9:0xB8,true);
            injector::WriteMemory<uintptr_t>(shared::base+site.rva+1,address,true);
            injector::WriteMemory<unsigned char>(shared::base+site.rva+5,0x90,true);
        }
        applyContinue=shared::base+0x592C6F;applySkip=shared::base+0x592CE2;
        changedContinue=shared::base+0x5B44E3;changedSkip=shared::base+0x5B4543;
        injector::MakeJMP(shared::base+0x592C69,Apply);
        injector::WriteMemory<unsigned char>(shared::base+0x592C6E,0x90,true);
        injector::MakeJMP(shared::base+0x5B44DD,Changed);
        injector::WriteMemory<unsigned char>(shared::base+0x5B44E2,0x90,true);
        injector::WriteMemory<uintptr_t>(shared::base+0x593AD7,reinterpret_cast<uintptr_t>(sounds.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A49B4,reinterpret_cast<uintptr_t>(presentation.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A47C1,reinterpret_cast<uintptr_t>(descriptions.data()),true);
        ready=true;RefreshPresentation();return true;
    }
}
