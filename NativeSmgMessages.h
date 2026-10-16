#pragma once
#include "NativeSmgMessagePolicy.h"
#include "GunRuntimeLog.h"
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>

namespace NativeSmgMessages
{
    using namespace NativeSmgMessagePolicy;
    using Lookup=int(__thiscall*)(void*,unsigned);
    using GetText=void*(__thiscall*)(void*,int,int);
    inline Lookup originalLookup=nullptr;
    inline GetText originalText=nullptr;
    inline std::array<unsigned,NativeSmgMessagePolicy::KeyCount> hashes{};
    struct Entry {unsigned text,parts,sequence,hash;};
    struct Bank {
        std::vector<Entry> entries;
        std::vector<std::pair<unsigned,unsigned>> ids;
        std::array<Message,KeyCount> custom{};
        unsigned installedOffset=0;
    };
    inline std::unordered_map<void*,std::unique_ptr<Bank>> banks;
    inline bool Extend(void* table)
    {
        auto* data=static_cast<unsigned char*>(table);
        auto* header=reinterpret_cast<unsigned*>(data);
        auto& storage=banks[table];
        if(storage && header[0]==storage->installedOffset)return true;
        if(!header[0] || header[1]>65536 || !header[8] || header[9]>65536)return false;
        if(!storage)storage=std::make_unique<Bank>();
        auto& bank=*storage;
        const auto* originalEntries=reinterpret_cast<const Entry*>(data+header[0]);
        bank.entries.assign(originalEntries,originalEntries+header[1]);
        std::map<unsigned,unsigned> indices;
        const auto* originalIds=reinterpret_cast<const unsigned*>(data+header[8]);
        for(unsigned i=0;i<header[9];++i)indices[originalIds[i*2]]=originalIds[i*2+1];
        unsigned added=0;
        for(unsigned i=0;i<KeyCount;++i) {
            if(!Generate(table,i,&bank.custom[i]))continue;
            const auto found=indices.find(hashes[i]);
            const unsigned index=found==indices.end()?unsigned(bank.entries.size()):found->second;
            const Entry entry{unsigned(reinterpret_cast<uintptr_t>(&bank.custom[i].text)-reinterpret_cast<uintptr_t>(table)),1,index+1,hashes[i]};
            if(index==bank.entries.size())bank.entries.push_back(entry);
            else bank.entries[index]=entry;
            indices[hashes[i]]=index;++added;
        }
        bank.ids.assign(indices.begin(),indices.end());
        bank.installedOffset=unsigned(reinterpret_cast<uintptr_t>(bank.entries.data())-reinterpret_cast<uintptr_t>(table));
        header[0]=bank.installedOffset;header[1]=unsigned(bank.entries.size());
        header[8]=unsigned(reinterpret_cast<uintptr_t>(bank.ids.data())-reinterpret_cast<uintptr_t>(table));
        header[9]=unsigned(bank.ids.size());
        GunRuntimeLog::Write("[WeaponUI] extended native bank=%p custom=%u/%u messages=%u",table,added,KeyCount,header[1]);
        return added!=0;
    }
    inline int __fastcall Find(void* table,void*,unsigned hash)
    {
        for(unsigned i=0;i<KeyCount;++i)if(hash==hashes[i]) {
            // A compiled catalogue already has the correct native fonts and
            // glyph metrics. Do not replace it with generated fallback text.
            if(originalLookup(table,hashes[0])<0 || originalLookup(table,hash)<0)Extend(table);
            break;
        }
        return originalLookup(table,hash);
    }
    inline void* __fastcall Fetch(void* table,void*,int index,int part)
    {
        // Real indices and native entries keep the renderer's count checks,
        // cache and direct message-table consumers valid.
        return originalText(table,index,part);
    }
    inline bool Install()
    {
        if(std::memcmp(reinterpret_cast<void*>(shared::base+0x8B1CD0),"\x83\xEC\x08\x8B\x41\x20",6) ||
           std::memcmp(reinterpret_cast<void*>(shared::base+0x8B1BF0),"\x8B\x01\x85\xC0",4)) return false;
        auto hash=reinterpret_cast<unsigned(__cdecl*)(const char*)>(shared::base+0xA03EA0);
        for(unsigned i=0;i<hashes.size();++i)hashes[i]=hash(Keys[i]);
        static SafeHook::Hook lookup(reinterpret_cast<void*>(shared::base+0x8B1CD0),reinterpret_cast<void*>(Find),true,reinterpret_cast<void**>(&originalLookup));
        static SafeHook::Hook fetch(reinterpret_cast<void*>(shared::base+0x8B1BF0),reinterpret_cast<void*>(Fetch),true,reinterpret_cast<void**>(&originalText));
        return true;
    }
}
