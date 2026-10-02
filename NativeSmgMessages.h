#pragma once
#include "NativeSmgMessagePolicy.h"

namespace NativeSmgMessages
{
    using namespace NativeSmgMessagePolicy;
    using Lookup=int(__thiscall*)(void*,unsigned);
    using GetText=void*(__thiscall*)(void*,int,int);
    inline Lookup originalLookup=nullptr;
    inline GetText originalText=nullptr;
    inline unsigned typeHash=0,descriptionHash=0,nameHash=0;
    inline int __fastcall Find(void* table,void*,unsigned hash)
    {
        if(hash==typeHash) return FirstIndex;
        if(hash==descriptionHash) return FirstIndex+1;
        if(hash==nameHash) return FirstIndex+2;
        return originalLookup(table,hash);
    }
    inline void* __fastcall Fetch(void* table,void*,int index,int part)
    {
        if(index>=FirstIndex && index<=FirstIndex+2)
        {const unsigned which=unsigned(index-FirstIndex);return part==0 && Generate(table,which)?&messages[which].text:nullptr;}
        return originalText(table,index,part);
    }
    inline bool Install()
    {
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x8B1CD0),"\x83\xEC\x08\x8B\x41\x20",6) ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0x8B1BF0),"\x8B\x01\x85\xC0",4)) return false;
        auto hash=reinterpret_cast<unsigned(__cdecl*)(const char*)>(shared::base+0xA03EA0);
        typeHash=hash(TypeKey);descriptionHash=hash(DescriptionKey);nameHash=hash(NameKey);
        static SafeHook::Hook lookup(reinterpret_cast<void*>(shared::base+0x8B1CD0),reinterpret_cast<void*>(Find),true,reinterpret_cast<void**>(&originalLookup));
        static SafeHook::Hook fetch(reinterpret_cast<void*>(shared::base+0x8B1BF0),reinterpret_cast<void*>(Fetch),true,reinterpret_cast<void**>(&originalText));
        return true;
    }
}
