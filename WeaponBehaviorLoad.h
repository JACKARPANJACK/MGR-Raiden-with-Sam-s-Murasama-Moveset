#pragma once

// Mid-instruction native guards: intentionally write EDI/ESI, preserve EAX,
// ECX, EDX, the x87 stack and ESP. The following native TEST sets branch flags.
namespace WeaponBehaviorLoad
{
    inline unsigned NativeAttachTarget = 0;
    // This specific native secondary-weapon call has already removed the old
    // constraint. A missing optional entity must skip construction of its handle.
    __declspec(naked) inline void AttachOptional()
    {
        __asm {
            cmp dword ptr [esp+8],0
            je missing
            cmp dword ptr [esp+0Ch],0
            je missing
            jmp dword ptr [NativeAttachTarget]
        missing:
            ret 14h
        }
    }
    __declspec(naked) inline void LoadEDI()
    {
        __asm {
            test eax,eax
            jz missing
            mov edi,dword ptr [eax+4F0h]
            ret
        missing:
            xor edi,edi
            ret
        }
    }
    __declspec(naked) inline void LoadESI()
    {
        __asm {
            test eax,eax
            jz missing
            mov esi,dword ptr [eax+4F0h]
            ret
        missing:
            xor esi,esi
            ret
        }
    }
}
