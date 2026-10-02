#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// Add messages to the existing native MCD renderer without replacing its font
// bank or any stock RPG messages. Only our two private event hashes/indices are
// intercepted; glyphs, textures, wrapping and layout remain native.
namespace NativeSmgMessagePolicy
{
    constexpr const char* TypeKey="codex_smg_type",*DescriptionKey="codex_smg_description";
    constexpr const char* NameKey="codex_smg_name",*Name="Kriss Vector SMG";
    constexpr const char* Type="Sub machine gun";
    constexpr std::array<const char*,3> Description{{
        "KRISS Vector submachine gun.",
        "Light rounds stagger and juggle foes.",
        "Hold the subweapon button and release",
    }};
    constexpr const char* LastLine="for a focused 30-round magazine dump.";
    constexpr int FirstIndex=0x70000000;
    struct Line {unsigned offset,padding,length,lengthWithEnd,height,horiz;};
    struct Text {unsigned offset,count,vpos,hpos,font;};
    struct Message {Text text{};std::array<Line,4> lines{};std::array<std::array<unsigned short,160>,4> content{};};
    inline std::array<Message,3> messages{};
    inline bool Generate(void* table,unsigned which)
    {
        auto* data=static_cast<unsigned char*>(table);
        const auto* header=reinterpret_cast<unsigned*>(data);
        if(!header[2] || !header[3] || header[3]>65536 || !header[6] || header[7]>64) return false;
        const char* lines[4]={which==1?Description[0]:which==2?Name:Type,Description[1],Description[2],LastLine};
        const unsigned count=which==1?4:1;
        const unsigned short* symbols=reinterpret_cast<unsigned short*>(data+header[2]);
        // Pick an available native font containing every requested ASCII glyph.
        // Font IDs and glyph indices differ across localized banks.
        std::array<unsigned short,128> glyphs{};
        unsigned font=0;bool found=false;
        for(unsigned f=0;f<header[7] && !found;++f)
        {
            font=*reinterpret_cast<unsigned*>(data+header[6]+f*20);
            glyphs.fill(0xFFFF);
            for(unsigned s=0;s<header[3];++s)
                if(symbols[s*4]==font && symbols[s*4+1]<128)
                    glyphs[symbols[s*4+1]]=static_cast<unsigned short>(*reinterpret_cast<const unsigned*>(symbols+s*4+2));
            found=true;
            for(unsigned l=0;l<count;++l) for(const char* p=lines[l];*p;++p)
                if(*p!=' ' && glyphs[static_cast<unsigned char>(*p)]==0xFFFF) found=false;
        }
        if(!found) return false;
        auto& m=messages[which];
        const uintptr_t base=reinterpret_cast<uintptr_t>(table);
        m.text={unsigned(reinterpret_cast<uintptr_t>(m.lines.data())-base),count,0,0,font};
        unsigned height=39;
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
            m.lines[l]={unsigned(reinterpret_cast<uintptr_t>(m.content[l].data())-base),0,characters,characters+1,height,0};
        }
        return true;
    }
}
