#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <cstdio>

// Clone boss timing/visual/audio tracks, adapting hits to Sam's player attack
// table. The archive remains immutable. Enemy flags and other actor-specific
// tracks cannot run on a player. Copies must outlive animation slots.
namespace SamBossSequence
{
    inline unsigned U16(const uint8_t* p) { return (p[0] << 8) | p[1]; }
    inline unsigned U32(const uint8_t* p) { return (U16(p) << 16) | U16(p + 2); }
    inline void W16(uint8_t* p, unsigned n) { p[0] = uint8_t(n >> 8); p[1] = uint8_t(n); }
    inline void W32(uint8_t* p, unsigned n) { W16(p, n >> 16); W16(p + 2, n); }
    inline size_t Size(const void* data)
    {
        if (!data) return 0;
        auto p = static_cast<const uint8_t*>(data);
        if (std::memcmp(p, "BXM\0", 4) && std::memcmp(p, "XML\0", 4)) return 0;
        unsigned nodes = U16(p + 8), pairs = U16(p + 10), strings = U32(p + 12);
        if (!nodes || nodes > 16384 || pairs > 32768 || strings > 65000) return 0;
        return 16 + nodes * 8 + pairs * 4 + strings;
    }
    inline unsigned AttackNumber(const char* code, unsigned bossNumber)
    {
        if (!code) return 12;
        // Native playable Sam entries: light 4/6/8, finisher 10,
        // heavy 12, charged draw 26. Preserve telegraphs as non-hits.
        if (!std::strcmp(code, "2200") || !std::strcmp(code, "2220")) return 4;
        if (!std::strcmp(code, "9100") || !std::strcmp(code, "9101") || !std::strcmp(code, "9102")) return 4;
        if (!std::strcmp(code, "9108")) return 12;
        if (!std::strcmp(code, "92e4")) return bossNumber == 14 ? 26 : 4;
        if (!std::strcmp(code, "2600") || !std::strcmp(code, "2610")) return 10;
        if (!std::strcmp(code, "3004") || !std::strcmp(code, "3024") ||
            !std::strcmp(code, "3017")) return 26;
        if (!std::strcmp(code, "3210")) return 4;
        return 12;
    }
    inline unsigned HitCount(const void* data)
    {
        if (!Size(data)) return 1;
        const auto* p = static_cast<const uint8_t*>(data);
        unsigned nodes=U16(p+8), pairs=U16(p+10), ps=16+nodes*8, ts=ps+pairs*4;
        unsigned count=0;
        for (unsigned i=0;i<nodes;++i)
        {
            const auto* n=p+16+i*8; unsigned ix=U16(n+6),attrs=U16(n+4);
            if (ix+attrs >= pairs) continue;
            const char* tag=reinterpret_cast<const char*>(p+ts+U16(p+ps+ix*4));
            if (std::strcmp(tag,"AttackTrack")) continue;
            for (unsigned c=U16(n+2);c<U16(n+2)+U16(n);++c)
            {
                const auto* child=p+16+c*8; unsigned ci=U16(child+6),ca=U16(child+4);
                for (unsigned a=1;a<=ca;++a)
                {
                    const auto* entry=p+ps+(ci+a)*4;
                    if (!std::strcmp(reinterpret_cast<const char*>(p+ts+U16(entry)),"No") &&
                        std::strtoul(reinterpret_cast<const char*>(p+ts+U16(entry+2)),nullptr,10)>0) ++count;
                }
            }
        }
        return count ? count : 1;
    }
    inline std::vector<uint8_t> RepairPlayableEffects(const void* data, size_t available)
    {
        const size_t size = Size(data);
        if (!size || size > available) return {};
        const auto* src = static_cast<const uint8_t*>(data);
        std::vector<uint8_t> out(src,src+size);
        const unsigned nodes=U16(src+8), pairs=U16(src+10), ps=16+nodes*8, ts=ps+pairs*4;
        const unsigned replacement=unsigned(out.size())-ts;
        const char release[]="119";
        out.insert(out.end(),release,release+sizeof(release));
        W32(out.data()+12,unsigned(out.size())-ts);
        for (unsigned i=0;i<nodes;++i)
        {
            const auto* node=src+16+i*8;
            unsigned ix=U16(node+6), attrs=U16(node+4), effectPair=0;
            if (ix+attrs>=pairs) return {};
            bool sam=false, missing=false;
            for (unsigned a=1;a<=attrs;++a)
            {
                const auto* entry=src+ps+(ix+a)*4;
                const char* key=reinterpret_cast<const char*>(src+ts+U16(entry));
                const char* value=reinterpret_cast<const char*>(src+ts+U16(entry+2));
                if (!std::strcmp(key,"EffDataId")) sam=!std::strcmp(value,"70656");
                if (!std::strcmp(key,"EffNo")) { effectPair=ix+a; missing=!std::strcmp(value,"146"); }
            }
            // Missing charged-combo recovery EST: retain timing and use Sam's
            // installed charged-release flash. Never modify archive memory.
            if (sam && missing) W16(out.data()+ps+effectPair*4+2,replacement);
        }
        return out;
    }
    inline std::vector<uint8_t> Adapt(const void* data, size_t available, const char* code = nullptr, bool raiden = false)
    {
        if (!data || available < 16) return {};
        size_t size = Size(data);
        if (!size || size > available) return {};
        auto src = static_cast<const uint8_t*>(data);
        std::vector<uint8_t> out(src, src + size);
        unsigned nodes = U16(src + 8), pairs = U16(src + 10);
        unsigned pairStart = 16 + nodes * 8, textStart = pairStart + pairs * 4;
        unsigned strings = U32(src + 12);
        auto append = [&](const char* s) {
            unsigned offset = unsigned(out.size()) - textStart;
            out.insert(out.end(), s, s + std::strlen(s) + 1);
            return offset;
        };
        unsigned zero = append("0"), always = append("4294967295");
        unsigned attackOffsets[27]{};
        for (unsigned n : {4u, 10u, 12u, 26u})
        {
            char number[12]; std::snprintf(number, sizeof(number), "%u", n);
            attackOffsets[n] = append(number);
        }
        W32(out.data() + 12, unsigned(out.size()) - textStart);
        auto str = [&](unsigned off) -> const char* {
            if (off >= strings || !std::memchr(src + textStart + off, 0, strings - off)) return "";
            return reinterpret_cast<const char*>(src + textStart + off);
        };
        for (unsigned i = 0; i < nodes; ++i)
        {
            auto node = out.data() + 16 + i * 8;
            unsigned children = U16(node), first = U16(node + 2), attrs = U16(node + 4), index = U16(node + 6);
            if (index + attrs >= pairs || first + children > nodes) return {};
            const char* tag = str(U16(src + pairStart + index * 4));
            bool attack = !std::strcmp(tag, "AttackTrack");
            bool retain = attack || !std::strcmp(tag, "SeqRoot") || !std::strcmp(tag, "Seq") ||
                !std::strcmp(tag, "EffectTrack") || !std::strcmp(tag, "SeTrack") ||
                !std::strcmp(tag, "VibTrack") || !std::strcmp(tag, "SpeedTrack");
            if (!retain)
            {
                W16(node, 0);
                for (unsigned a = 1; a <= attrs; ++a)
                    if (!std::strcmp(str(U16(src + pairStart + (index + a) * 4)), "SeqNum"))
                        W16(out.data() + pairStart + (index + a) * 4 + 2, zero);
            }
            const bool effect = !std::strcmp(tag,"EffectTrack");
            // Raiden's own hit numbers, layer gates, audio and effects remain
            // native. Flags belonging to his action graph are excluded above
            // because the addon owns its duration and state transitions.
            if (raiden || (!attack && !effect)) continue;
            for (unsigned c = first; c < first + children; ++c)
            {
                const uint8_t* child = src + 16 + c * 8;
                unsigned ci = U16(child + 6), ca = U16(child + 4);
                if (ci + ca >= pairs) return {};
                bool telegraph = false;
                unsigned bossNumber = 0;
                unsigned start = zero;
                for (unsigned a = 1; a <= ca; ++a)
                {
                    const uint8_t* entry = src + pairStart + (ci + a) * 4;
                    const char* name = str(U16(entry));
                    if (!std::strcmp(name, "No"))
                    {
                        bossNumber = unsigned(std::strtoul(str(U16(entry + 2)), nullptr, 10));
                        telegraph = bossNumber == 0;
                    }
                    if (!std::strcmp(name, "StartTime")) start = U16(entry + 2);
                }
                for (unsigned a = 1; a <= ca; ++a)
                {
                    const char* name = str(U16(src + pairStart + (ci + a) * 4));
                    if (effect)
                    {
                        // Boss layers 1/2 gate leap VFX out on the player.
                        // Preserve bank, effect, time, offsets and control type.
                        if (!std::strcmp(name,"LayerFlag"))
                            W16(out.data()+pairStart+(ci+a)*4+2,always);
                        continue;
                    }
                    if (!std::strcmp(name, "No")) W16(out.data() + pairStart + (ci + a) * 4 + 2,
                        telegraph ? zero : attackOffsets[AttackNumber(code, bossNumber)]);
                    if (!std::strcmp(name, "LayerFlag")) W16(out.data() + pairStart + (ci + a) * 4 + 2, always);
                    // Boss proximity/telegraph boxes are not damaging strikes.
                    if (telegraph && !std::strcmp(name, "EndTime"))
                        W16(out.data() + pairStart + (ci + a) * 4 + 2, start);
                }
            }
        }
        return out;
    }
}
