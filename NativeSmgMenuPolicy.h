#pragma once
namespace NativeSmgMenuPolicy
{
    constexpr unsigned Alias=0x4D534D47; // Mod-owned item alias, independent of native guns.
    // 1..46 includes base/DLC collectibles; 47 is unused in all three item lists.
    constexpr unsigned ItemId=47, Slot=11;
    constexpr int Entry=10, Capacity=11;
    inline int Insert(int* entries,int* amounts,int* maxima,int count)
    {
        if(count<1 || count>Capacity) return count;
        for(int i=0;i<count;++i) if(entries[i]==Entry) return count;
        // The native builder appends None. Never overwrite an owned stock item.
        if(entries[count-1]!=-1) return count;
        const int index=count-1;
        entries[index]=Entry; amounts[index]=30; maxima[index]=30;
        if(count<Capacity)
        {entries[count]=-1;amounts[count]=maxima[count]=0;++count;}
        return count;
    }
    inline int Find(const int* entries,int count)
    {
        if(count<1 || count>Capacity) return -1;
        for(int i=0;i<count;++i) if(entries[i]==Entry) return i;
        return -1;
    }
    inline int RowIndex(int cursor,int count,int row)
    {return count>0 && count<=Capacity ? ((cursor-5+row)%count+count)%count : -1;}
}
