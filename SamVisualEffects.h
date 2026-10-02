#pragma once
#include <Pl0000.h>
namespace SamVisualEffects
{
    inline int ChargeNumber(int number)
    {
        switch (number)
        {
        case 539: return 117;
        case 540: return 118;
        case 541: return 119;
        case 550: return 121;
        default: return number;
        }
    }
    inline int Bank(int number)
    {
        static constexpr unsigned boss[] = {
            104, 149, 247, 249, 250, 251, 252, 253, 254, 255,
            325, 326, 327, 328, 329, 330, 331, 333, 336, 340, 341, 519
        };
        for (unsigned n : boss) if (number == int(n)) return 0x20020;
        static constexpr unsigned sam[] = {0,1,2,3,7,8,9,10,11,15,16,17,18,19,20,21,22,23,24,25,26,28,29,30,31,34,35,36,37,38,39,40,41,42,43,44,45,46,50,51,52,54,56,57,58,59,62,63,64,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,105,106,107,108,109,110,111,113,114,115,117,118,119,121,122,123,124,125,129,130,132,133,134,135,136,137,139,140,141,142,143,144,145,153,167,168,169,170,171,172,178,179,180,181,185,186,187,188,198,199,200,201,205,206,207,208,209,217,219,300,301,302,310,311,711,712,713,714,715,716,717,718,719,720,721,722,723,724,725,726,727,728,729,730,731,732,733,734,735};
        static constexpr unsigned core[] = {0,1,2,3,4,5,6,7,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,75,77,78,79,80,81,83,84,85,86,87,88,90,93,94,97,98,99,100,105,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,139,140,141,142,143,145,146,147,148,150,151,152,153,155,156,157,158,160,161,162,163,165,166,167,168,169,170,171,172,173,174,175,180,181,182,183,184,185,192,193,194,195,196,197,199,200,201,202,203,204,205,206,207,230,231,233,240,241,242,243,246,247,248,249,260,263,264,265,266,267,268,269,270,271,272,276,277,278,279,283,284,285,286,287,288,290,292,293,294,302,310,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,337,338,339,340,341,342,343,344,345,346,347,350,354,355,356,362,363,364,365,366,400,401,402,403,405,406,408,409,412,413,415,416,417,418,419,420,421,422,423,424,425,426,430,431,432,433,434,435,436,440,441,442,443,444,445,446};
        for (unsigned n : sam) if (number == int(n)) return 0x11400;
        for (unsigned n : core) if (number == int(n)) return 0x7C0000;
        return -1;
    }
    inline void Spawn(Behavior* owner, Entity* anchor, int bank, int number, cEspControler* controller, Entity* blade = nullptr)
    {
        if (!owner || !anchor) return;
        alignas(16) unsigned char info[0x150]{};
        const int parent = reinterpret_cast<int(__thiscall*)(Entity*)>(shared::base + 0x67C8A0)(anchor);
        reinterpret_cast<void*(__thiscall*)(void*,int,Behavior*,int)>(shared::base + 0x39A0)(info,number,owner,parent);
        reinterpret_cast<void(__thiscall*)(void*,cEspControler*)>(shared::base + 0x9FFB20)(info,controller);
        reinterpret_cast<void(__thiscall*)(void*,Entity*,int)>(shared::base + 0xA03080)(info,anchor,0);
        if (blade) reinterpret_cast<void(__thiscall*)(void*,Entity*,int)>(shared::base + 0xA03080)(info,blade,1);
        reinterpret_cast<void(__thiscall*)(Behavior*,int,void*)>(shared::base + 0x68C8B0)(owner,bank,info);
    }
}
