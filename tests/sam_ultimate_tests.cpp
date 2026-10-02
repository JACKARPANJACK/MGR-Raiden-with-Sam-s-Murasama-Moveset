#include "../SamUltimatePolicy.h"
#include "../SamBossSequence.h"
#include "../SamTogglePolicy.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main()
{
    using namespace SamUltimatePolicy;
    assert(ControllerButton==0x20 && SelectButton==0x200);
    assert(!(ControllerButton&SelectButton));
    assert(Press(ControllerButton,ControllerButton,0));
    assert(!Press(ControllerButton,ControllerButton,ControllerButton));
    assert(!Press(SelectButton,SelectButton,0));
    assert(ReserveButton(0x100000,true,false,false,false,false));
    assert(ReserveButton(0x10000F,true,false,false,false,false));
    assert(!ReserveButton(0x100000,true,false,true,false,false));
    assert(!ReserveButton(0x100000,true,false,false,true,false));
    assert(!ReserveButton(0x100000,true,false,false,false,true));
    assert(!ReserveButton(0x100000,true,true,false,false,false));
    assert(!ReserveButton(0x100007,true,false,false,false,false));
    assert(SamTogglePolicy::OwnsAction(Action));
    assert(CanStart(0x100000, true, false, false));
    assert(!CanStart(0x10000F, true, false, false));
    assert(CanStart(0x10000F, true, false, true));
    for (auto state : {0x100004u, 0x100007u, 0x100066u, 0x47u})
        assert(!CanStart(state, true, false, true));
    assert(!CanStart(0x100000, false, false, true));
    assert(!CanStart(0x100000, true, true, true));
    Queue q;
    q.Request(); q.Tick(0x10000F, true, false); assert(q.pending);
    q.Tick(0x100000, true, false); assert(q.pending);
    q.Tick(0x100066, true, false); assert(!q.pending);
    q.Request(); q.Tick(0x10000F, true, true); assert(!q.pending);
    q.Request(); q.Reset(); assert(!q.pending);
    q.Request(); for (unsigned i = 0; i <= 360; ++i) q.Tick(0x10000F, true, false);
    assert(!q.pending);
    unsigned checked = 0;
    unsigned raidenStages=0;
    for (const auto& move : Moves) for (const char* code : {move.windup, move.release})
    {
        if (!code) continue;
        std::string base = move.raiden ?
            "local_assets/data000/pl/pl0010.dat.unpacked/pl0010_" + std::string(code) :
            (std::ifstream("local_assets/data000/em/em0020.dat.unpacked/em0020_" + std::string(code) + ".mot", std::ios::binary).good() ?
             "local_assets/data000/em/em0020.dat.unpacked/em0020_" + std::string(code) :
             "local_assets/data107/pl/pl1400.dat.unpacked/pl1400_" + std::string(code));
        assert(std::ifstream(base + ".mot", std::ios::binary).good());
        std::ifstream file(base + "_0_seq.bxm", std::ios::binary);
        if (!file.good()) file.open(base + "_2_seq.bxm", std::ios::binary);
        std::vector<uint8_t> source((std::istreambuf_iterator<char>(file)), {});
        assert(!source.empty());
        const auto original = source;
        auto adapted = SamBossSequence::Adapt(source.data(), source.size(), code, move.raiden);
        assert(!adapted.empty() && source == original);
        assert(SamBossSequence::Size(adapted.data()) == adapted.size());
        if (move.raiden)
        {
            ++raidenStages;
            std::ifstream map("local_assets/data000/pl/pl0010.dat.unpacked/pl0010_animationMap.bxm",std::ios::binary);
            std::vector<uint8_t> names((std::istreambuf_iterator<char>(map)),{});
            assert(!names.empty());
            const std::string strings(reinterpret_cast<const char*>(names.data()),names.size());
            assert(strings.find(code)!=std::string::npos);
        }
        assert(SamBossSequence::Adapt(source.data(), source.size() - 1).empty());
        using namespace SamBossSequence;
        unsigned nodes = U16(adapted.data() + 8), pairs = U16(adapted.data() + 10);
        unsigned ps = 16 + nodes * 8, ts = ps + pairs * 4;
        auto str = [&](unsigned off) { return reinterpret_cast<const char*>(adapted.data() + ts + off); };
        for (unsigned i = 0; i < nodes; ++i)
        {
            auto n = adapted.data() + 16 + i * 8;
            unsigned index = U16(n + 6);
            const char* tag = str(U16(adapted.data() + ps + index * 4));
            if (move.raiden && (!std::strcmp(tag,"EffectTrack") || !std::strcmp(tag,"SeTrack") ||
                !std::strcmp(tag,"SpeedTrack") || !std::strcmp(tag,"VibTrack")))
            {
                // Native player banks and timing must not be converted to Sam.
                assert(!std::memcmp(n,source.data()+16+i*8,8));
                const unsigned first=U16(n+2), count=U16(n);
                for (unsigned child=first;child<first+count;++child)
                {
                    const auto* cn=adapted.data()+16+child*8;
                    const unsigned ci=U16(cn+6),attrs=U16(cn+4);
                    assert(!std::memcmp(adapted.data()+ps+ci*4,source.data()+ps+ci*4,(attrs+1)*4));
                }
            }
            if (!std::strcmp(tag, "FlagsTrack")) assert(U16(n) == 0);
            if (std::strcmp(tag, "AttackTrack")) continue;
            for (unsigned c = U16(n + 2); c < U16(n + 2) + U16(n); ++c)
            {
                auto child = adapted.data() + 16 + c * 8;
                unsigned ci = U16(child + 6);
                for (unsigned a = 1; a <= U16(child + 4); ++a)
                {
                    auto entry = adapted.data() + ps + (ci + a) * 4;
                    if (!std::strcmp(str(U16(entry)), "No"))
                    {
                        const auto originalEntry = source.data() + ps + (ci + a) * 4;
                        unsigned originalNo = unsigned(std::strtoul(
                            reinterpret_cast<const char*>(source.data()+ts+U16(originalEntry+2)), nullptr, 10));
                        unsigned actual = unsigned(std::strtoul(str(U16(entry+2)), nullptr, 10));
                        unsigned expected = move.raiden ? originalNo : originalNo == 0 ? 0 : AttackNumber(code, originalNo);
                        assert(actual == expected);
                        if (!std::strcmp(code,"2220") || !std::strcmp(code,"9100")) assert(actual == 0 || actual == 4);
                        if (!std::strcmp(code,"9108")) assert(actual == 0 || actual == 12);
                        if (!std::strcmp(code,"3004")) assert(actual == 0 || actual == 26);
                    }
                }
            }
        }
        ++checked;
    }
    assert(checked == 25);
    assert(raidenStages == 4);
    assert(SamBossSequence::AttackNumber("92e4",30)==4);
    assert(SamBossSequence::AttackNumber("92e4",14)==26);
    assert(SamBossSequence::AttackNumber("a649",9)==12);
    assert(SamBossSequence::AttackNumber("2610",9)==10);
    assert(SamBossSequence::Adapt(nullptr, 0).empty());
    std::cout << "PASS: X queue, interruption/timeout guards, 25 stages, Raiden attack IDs preserved and boss attacks adapted\n";
}
