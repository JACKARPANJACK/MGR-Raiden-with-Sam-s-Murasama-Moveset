#pragma once
#include "PistolPolicy.h"
namespace NativeSmgMenuPolicy
{
    constexpr unsigned Alias=0x4D534D47; // Mod-owned item alias, independent of native guns.
    // 1..46 includes base/DLC collectibles; 47 is unused in all three item lists.
    constexpr unsigned ItemId=47, Slot=11; // Existing assault rifle.
    constexpr unsigned VectorAlias=0x4D535647, VectorItemId=48, VectorSlot=12;
    constexpr int VectorEntry=11;
    constexpr bool GunSlot(int slot) {return slot==Slot || slot==VectorSlot || PistolPolicy::Slot(slot);}
    constexpr int Entry=10, Capacity=15, GunCount=4;
    constexpr unsigned SlotFor(unsigned gun) {return 11+gun;}
    constexpr unsigned IndexFor(int slot) {return GunSlot(slot)?unsigned(slot-11):0;}
    constexpr unsigned AliasFor(unsigned gun) {return gun==0?Alias:gun==1?VectorAlias:PistolPolicy::Get(gun-2).alias;}
    constexpr unsigned ItemFor(unsigned gun) {return gun==0?ItemId:gun==1?VectorItemId:PistolPolicy::Get(gun-2).item;}
    constexpr unsigned MagazineFor(unsigned gun) {return gun<2?30:PistolPolicy::Get(gun-2).magazine;}
    inline int Insert(int* entries,int* amounts,int* maxima,int count,int entry=Entry)
    {
        if(count<1 || count>=Capacity) return count;
        for(int i=0;i<count;++i) if(entries[i]==entry) return count;
        // The native builder appends None. Never overwrite an owned stock item.
        if(entries[count-1]!=-1) return count;
        const int index=count-1;
        entries[index]=entry; amounts[index]=MagazineFor(unsigned(entry-Entry)); maxima[index]=amounts[index];
        if(count<Capacity)
        {entries[count]=-1;amounts[count]=maxima[count]=0;++count;}
        return count;
    }
    inline int Find(const int* entries,int count,int entry=Entry)
    {
        if(count<1 || count>Capacity) return -1;
        for(int i=0;i<count;++i) if(entries[i]==entry) return i;
        return -1;
    }
    inline int RowIndex(int cursor,int count,int row)
    {return count>0 && count<=Capacity ? ((cursor-5+row)%count+count)%count : -1;}
}
