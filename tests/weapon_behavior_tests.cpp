#include "../WeaponBehaviorLoad.h"
#include <cassert>
#include <array>
#include <cstring>
#include <iostream>
using WeaponBehaviorLoad::LoadEDI;
using WeaponBehaviorLoad::LoadESI;
static unsigned attachmentCalls=0, attachmentThis=0, attachmentChild=0;
__declspec(naked) void AttachmentTarget()
{
    __asm {
        inc attachmentCalls
        mov attachmentThis,ecx
        mov eax,dword ptr [esp+0Ch]
        mov attachmentChild,eax
        ret 14h
    }
}
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
    WeaponBehaviorLoad::NativeAttachTarget=reinterpret_cast<unsigned>(&AttachmentTarget);
    using Attach=void(__thiscall*)(void*,int,void*,void*,int,int);
    auto attach=reinterpret_cast<Attach>(&WeaponBehaviorLoad::AttachOptional);
    unsigned actor=0,parent=0,child=0;
    for(int i=0;i<10000;++i)
    {
        const unsigned before=attachmentCalls;
        attach(&actor,5,nullptr,nullptr,0x701,-1);
        attach(&actor,5,&parent,nullptr,0x701,-1);
        attach(&actor,5,nullptr,&child,0x701,-1);
        assert(attachmentCalls==before);
        attach(&actor,5,&parent,&child,0x701,-1);
        assert(attachmentCalls==before+1);
        assert(attachmentThis==reinterpret_cast<unsigned>(&actor));
        assert(attachmentChild==reinterpret_cast<unsigned>(&child));
    }
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
