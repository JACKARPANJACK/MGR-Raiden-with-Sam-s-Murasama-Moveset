#pragma once

// Mid-instruction native guards: intentionally write EDI/ESI, preserve EAX,
// ECX, EDX, the x87 stack and ESP. The following native TEST sets branch flags.
namespace WeaponBehaviorLoad
{
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
