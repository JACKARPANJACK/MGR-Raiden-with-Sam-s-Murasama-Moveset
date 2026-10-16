#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// Retail primary weapon 9 is pl0102, Raiden's R00 blade. The stock menu
// has no row for it, although native streaming and equipping support it.
namespace NativeSwordMenu
{
    constexpr int Entry=11, Weapon=9, Capacity=11;
    constexpr int RowIndex(int cursor,int count,int row)
    {return count>0 && count<=Capacity ? ((cursor-5+row)%count+count)%count : -1;}
    inline bool ready=false;
    inline std::array<uintptr_t,12> apply{},changed{};
    inline std::array<unsigned,12> sounds{};
    inline std::array<uintptr_t,36> presentation{},descriptions{};
    inline uintptr_t applyContinue=0,changedContinue=0;
    inline int& Field(void* menu,unsigned offset)
    {return *reinterpret_cast<int*>(static_cast<char*>(menu)+offset);}
    inline int* Entries(void* menu)
    {return reinterpret_cast<int*>(static_cast<char*>(menu)+0x400);}
    inline int Find(void* menu)
    {
        const int count=Field(menu,0x3B0);
        if(count<1 || count>Capacity)return -1;
        for(int i=0;i<count;++i)if(Entries(menu)[i]==Entry)return i;
        return -1;
    }
    inline void Build(void* menu)
    {
        if(!ready || Find(menu)>=0)return;
        int& count=Field(menu,0x3B0);
        // No None sentinel in the primary list. Never cross into amounts.
        if(count>0 && count<Capacity)Entries(menu)[count++]=Entry;
    }
    __declspec(naked) inline void Apply()
    {
        __asm {mov esi,9
            jmp dword ptr [applyContinue]}
    }
    __declspec(naked) inline void Changed()
    {
        __asm {mov eax,9
            jmp dword ptr [changedContinue]}
    }
    inline bool Bytes(unsigned rva,const char* bytes,size_t size)
    {return !std::memcmp(reinterpret_cast<void*>(shared::base+rva),bytes,size);}
    inline bool Install()
    {
        if(!Bytes(0x592BC5,"\x83\xF8\x0A\x77\x4D\xFF\x24\x85",8) ||
           !Bytes(0x5B4473,"\x83\xF8\x0A\x77\x4A\xFF\x24\x85",8) ||
           !Bytes(0x5A49A8,"\xC7\x44\x24\x44",4) ||
           !Bytes(0x5A479F,"\x8D\x1C\x40\x8D\x1C\x9D",6) ||
           !Bytes(0x593B34,"\x8B\x04\x8D",3))return false;
        const auto pointer=[](unsigned at,unsigned target) {
            return *reinterpret_cast<uintptr_t*>(shared::base+at)==shared::base+target;
        };
        if(!pointer(0x592BCD,0x592D10) || !pointer(0x5B447B,0x5B467C) ||
           !pointer(0x5A49AC,0x14B57F0) || !pointer(0x5A47A5,0x12B54E8) ||
           !pointer(0x593B37,0x1256940) ||
           *reinterpret_cast<unsigned*>(shared::base+0x14A99C0+Weapon*4)!=0x10102)return false;
        std::memcpy(apply.data(),reinterpret_cast<void*>(shared::base+0x592D10),44);
        std::memcpy(changed.data(),reinterpret_cast<void*>(shared::base+0x5B467C),44);
        std::memcpy(sounds.data(),reinterpret_cast<void*>(shared::base+0x1256940),44);
        std::memcpy(descriptions.data(),reinterpret_cast<void*>(shared::base+0x12B54E8),132);
        std::memcpy(presentation.data(),reinterpret_cast<void*>(shared::base+0x14B57F0),132);
        std::memcpy(presentation.data()+33,presentation.data(),12);
        presentation[33]=reinterpret_cast<uintptr_t>("mod_r00_short");
        presentation[34]=0x517db537;
        presentation[35]=0;
        std::memcpy(descriptions.data()+33,descriptions.data(),12);
        descriptions[33]=reinterpret_cast<uintptr_t>("mod_r00_name");
        descriptions[34]=reinterpret_cast<uintptr_t>("mod_r00_desc");
        descriptions[35]=0;
        applyContinue=shared::base+0x592C19;changedContinue=shared::base+0x5B44C4;
        apply[Entry]=reinterpret_cast<uintptr_t>(Apply);
        changed[Entry]=reinterpret_cast<uintptr_t>(Changed);sounds[Entry]=sounds[0];
        injector::WriteMemory<unsigned char>(shared::base+0x592BC7,Entry,true);
        injector::WriteMemory<unsigned char>(shared::base+0x5B4475,Entry,true);
        injector::WriteMemory<uintptr_t>(shared::base+0x592BCD,reinterpret_cast<uintptr_t>(apply.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5B447B,reinterpret_cast<uintptr_t>(changed.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A49AC,reinterpret_cast<uintptr_t>(presentation.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x5A47A5,reinterpret_cast<uintptr_t>(descriptions.data()),true);
        injector::WriteMemory<uintptr_t>(shared::base+0x593B37,reinterpret_cast<uintptr_t>(sounds.data()),true);
        return ready=true;
    }
    inline void RefreshPresentation()
    {
        if(!ready)return;
        std::memcpy(presentation.data(),reinterpret_cast<void*>(shared::base+0x14B57F0),132);
        std::memcpy(presentation.data()+33,presentation.data(),12);
        presentation[33]=reinterpret_cast<uintptr_t>("mod_r00_short");
        presentation[34]=0x517db537;
        presentation[35]=0;
    }

}
