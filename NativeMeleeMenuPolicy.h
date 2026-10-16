#pragma once
#include <array>
namespace NativeMeleeMenuPolicy
{
    constexpr int Capacity=7,UnarmedEntry=3,DualEntry=4,IaidoEntry=5;
    // WeaponSwitcher indices, never native custom-weapon table indices.
    constexpr int Selection(int entry)
    {return entry==0?1:entry==1?2:entry==2?3:entry==3?6:entry==4?4:entry==5?5:entry==-1?0:-1;}
    constexpr int Entry(int selection)
    {return selection==1?0:selection==2?1:selection==3?2:selection==6?3:selection==4?4:selection==5?5:-1;}
    inline int Insert(int* entries,int count)
    {
        if(count<1 || count>4 || entries[count-1]!=-1)return count;
        for(int i=0;i<count-1;++i)if(entries[i]>=UnarmedEntry)return count;
        for(int entry:{UnarmedEntry,DualEntry,IaidoEntry})entries[count++-1]=entry;
        entries[count-1]=-1;return count;
    }
    constexpr int RowIndex(int cursor,int count,int row)
    {return count>0 && count<=Capacity?((cursor-5+row)%count+count)%count:-1;}
    constexpr const char* Caption(int entry)
    {return entry==3?"Unarmed":entry==4?"Dual Swords":entry==5?"Iaido katana":nullptr;}
}
