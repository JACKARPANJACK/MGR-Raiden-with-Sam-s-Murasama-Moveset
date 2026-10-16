#pragma once
#include "NativeSmgMenuPolicy.h"
#include <array>
#include <cstring>
#include <cstdint>

// Three native 11-cell arrays cannot hold ten stock entries, two firearms
// and None. Relocate every verified native consumer into owned storage.
namespace NativeFirearmMenuStorage {
inline std::array<int,NativeSmgMenuPolicy::Capacity> entries{},amounts{},maxima{};
struct Site {unsigned rva;const char* bytes;unsigned size,field;bool lea;};
inline constexpr Site Sites[]={
    {0x5924fe,"\x8D\x86\x38\x04\x00\x00",6,0x438,true},
    {0x59295c,"\x8D\x96\xC4\x03\x00\x00",6,0x3c4,true},
    {0x592b07,"\x8B\x84\x8F\xC4\x03\x00\x00",7,0x3c4,false},
    {0x5931ae,"\x89\x8C\x86\xC4\x03\x00\x00",7,0x3c4,false},
    {0x5931d6,"\x89\x84\x96\x38\x04\x00\x00",7,0x438,false},
    {0x5931f0,"\x89\x84\x8E\x70\x04\x00\x00",7,0x470,false},
    {0x5931ff,"\x89\x9C\x96\x38\x04\x00\x00",7,0x438,false},
    {0x59320c,"\x89\x9C\x86\x70\x04\x00\x00",7,0x470,false},
    {0x593235,"\x89\x84\x8E\x38\x04\x00\x00",7,0x438,false},
    {0x59324b,"\x89\x84\x8E\x70\x04\x00\x00",7,0x470,false},
    {0x59326f,"\xC7\x84\x96\xC4\x03\x00\x00\xFF\xFF\xFF\xFF",11,0x3c4,false},
    {0x593280,"\x89\x9C\x86\x38\x04\x00\x00",7,0x438,false},
    {0x59328d,"\x89\x9C\x8E\x70\x04\x00\x00",7,0x470,false},
    {0x593a86,"\x8B\x8C\x91\xC4\x03\x00\x00",7,0x3c4,false},
    {0x5a4777,"\x8B\x84\x8E\xC4\x03\x00\x00",7,0x3c4,false},
    {0x5a4873,"\x83\xBC\x86\xC4\x03\x00\x00\x02",8,0x3c4,false},
    {0x5a490d,"\x8D\x97\x38\x04\x00\x00",6,0x438,true},
    {0x5a4917,"\x8D\x8F\xC4\x03\x00\x00",6,0x3c4,true},
    {0x5a4921,"\x8D\x97\x70\x04\x00\x00",6,0x470,true},
    {0x5b43e7,"\x8B\x84\x8E\xC4\x03\x00\x00",7,0x3c4,false},
};
inline uintptr_t Address(unsigned field) {
    return reinterpret_cast<uintptr_t>(field==0x3C4?entries.data():field==0x438?amounts.data():maxima.data());
}
inline bool Validate() {
    for(const auto& site:Sites)if(std::memcmp(reinterpret_cast<void*>(shared::base+site.rva),site.bytes,site.size))return false;
    return true;
}
inline void Install() {
    entries.fill(-1);amounts.fill(0);maxima.fill(0);
    for(const auto& site:Sites) {
        if(site.lea) {
            const unsigned char reg=(static_cast<unsigned char>(site.bytes[1])>>3)&7;
            injector::WriteMemory<unsigned char>(shared::base+site.rva,0xB8+reg,true);
            injector::WriteMemory<uintptr_t>(shared::base+site.rva+1,Address(site.field),true);
            injector::WriteMemory<unsigned char>(shared::base+site.rva+5,0x90,true);
        } else {
            injector::WriteMemory<unsigned char>(shared::base+site.rva+1,(site.bytes[1]&0x38)|4,true);
            injector::WriteMemory<unsigned char>(shared::base+site.rva+2,(site.bytes[2]&0xF8)|5,true);
            injector::WriteMemory<uintptr_t>(shared::base+site.rva+3,Address(site.field),true);
        }
    }
}
}
