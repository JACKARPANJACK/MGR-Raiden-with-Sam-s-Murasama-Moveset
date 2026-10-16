#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// Add messages to the existing native MCD renderer without replacing its font
// bank or any stock RPG messages. Only our private event hashes/indices are
// intercepted; glyphs, textures, wrapping and layout remain native.
namespace NativeSmgMessagePolicy
{
    constexpr const char* TypeKey="codex_smg_type",*DescriptionKey="codex_smg_description";
    constexpr const char* NameKey="codex_smg_name",*Name="KRISS Vector SMG";
    constexpr int FirstIndex=0x70000000;
    struct Line {unsigned offset,padding,length,lengthWithEnd,height,horiz;};
    struct Text {unsigned offset,count,vpos,hpos,font;};
    struct Message {Text text{};std::array<Line,4> lines{};std::array<std::array<unsigned short,160>,4> content{};};
    #include "NativeWeaponMessages.generated.h"
    inline std::array<Message,KeyCount> messages{};
    inline bool Generate(void* table,unsigned which,Message* destination=nullptr)
    {
        if(which>=messages.size())return false;
        auto* data=static_cast<unsigned char*>(table);
        const auto* header=reinterpret_cast<unsigned*>(data);
        if(!header[2] || !header[3] || header[3]>65536 || !header[6] || header[7]>64) return false;
        const auto& lines=KeyLines[which];
        const unsigned count=LineCounts[which];
        const unsigned short* symbols=reinterpret_cast<unsigned short*>(data+header[2]);
        // Keep retail UI typography: long item labels use font 10, body
        // descriptions font 3, and abbreviated HUD labels built-in font 0.
        std::array<unsigned short,128> glyphs{};
        glyphs.fill(0xFFFF);
        unsigned font=MessageFonts[which];
        bool available=false;
        for(unsigned f=0;f<header[7];++f)
            if(*reinterpret_cast<unsigned*>(data+header[6]+f*20)==font)available=true;
        if(!available)font=3;
        for(unsigned s=0;s<header[3];++s)
            if(symbols[s*4]==font && symbols[s*4+1]<128)
                glyphs[symbols[s*4+1]]=static_cast<unsigned short>(*reinterpret_cast<const unsigned*>(symbols+s*4+2));
        if(font==0) {
            for(unsigned c='A';c<='Z';++c)glyphs[c]=static_cast<unsigned short>(0x400f+c-'A');
            for(unsigned c='0';c<='9';++c)glyphs[c]=static_cast<unsigned short>(0x4000+c-'0');
        }
        if(font==10 && glyphs['0']==0xFFFF)
            for(unsigned s=0;s<header[3];++s)
                if(symbols[s*4]==3 && symbols[s*4+1]=='0')glyphs['0']=static_cast<unsigned short>(symbols[s*4+2]);
        bool found=true;
        for(unsigned l=0;l<count;++l)for(const char* p=lines[l];*p;++p)
            if(*p!=' ' && glyphs[static_cast<unsigned char>(*p)]==0xFFFF)found=false;
        if(!found) return false;
        auto& m=destination?*destination:messages[which];
        const uintptr_t base=reinterpret_cast<uintptr_t>(table);
        m.text={unsigned(reinterpret_cast<uintptr_t>(m.lines.data())-base),count,0,0,font};
        unsigned height=39,total=0;
        for(unsigned f=0;f<header[7];++f)
            if(*reinterpret_cast<unsigned*>(data+header[6]+f*20)==font)
            {height=unsigned(*reinterpret_cast<float*>(data+header[6]+f*20+8));break;}
        for(unsigned l=0;l<count;++l)
        {
            unsigned n=0;
            for(const char* p=lines[l];*p;++p)
            {
                if(n+3>=m.content[l].size()) return false;
                m.content[l][n++]=*p==' '?0x8001:glyphs[static_cast<unsigned char>(*p)];
                m.content[l][n++]=*p==' '?static_cast<unsigned short>(font):0;
            }
            m.content[l][n++]=0x8000;
            // MGR counts characters (each has a kerning word), not ushort
            // tokens, and uses integer line height. The end marker adds one.
            const unsigned characters=(n-1)/2;
            m.lines[l]={unsigned(reinterpret_cast<uintptr_t>(m.content[l].data())-base),total,characters,characters+1,height,0};
            total+=characters;
        }
        m.text.hpos=total;
        return true;
    }
}
