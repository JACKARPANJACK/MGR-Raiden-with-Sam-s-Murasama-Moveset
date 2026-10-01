#include "../WeaponBehaviorLoad.h"
#include <cassert>
#include <array>
#include <cstring>
#include <iostream>
using WeaponBehaviorLoad::LoadEDI;
using WeaponBehaviorLoad::LoadESI;
__declspec(naked) unsigned __cdecl ReadEDI(void*)
{
    __asm {
        push edi
        mov eax,dword ptr [esp+8]
        call LoadEDI
        mov eax,edi
        pop edi
        ret
    }
}
__declspec(naked) unsigned __cdecl ReadESI(void*)
{
    __asm {
        push esi
        mov eax,dword ptr [esp+8]
        call LoadESI
        mov eax,esi
        pop esi
        ret
    }
}
int main()
{
    assert(ReadEDI(nullptr)==0 && ReadESI(nullptr)==0);
    std::array<unsigned char,0x4F4> behavior{};
    assert(ReadEDI(behavior.data())==0 && ReadESI(behavior.data())==0);
    const unsigned entity=0x12345678;
    std::memcpy(behavior.data()+0x4F0,&entity,4);
    for(int i=0;i<10000;++i)
    {
        assert(ReadEDI(behavior.data())==entity && ReadESI(behavior.data())==entity);
        assert(ReadEDI(nullptr)==0 && ReadESI(nullptr)==0);
    }
    std::cout<<"PASS: actual x86 optional weapon guards handle null/non-null Behaviors and preserve stack across repeated calls.\n";
}
