#include "../SamUltimatePolicy.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <fstream>
#include <iterator>
#include "../SamBossSequence.h"
int main()
{
    using namespace LightningSpecialPolicy;
    SamUltimatePolicy::EnsureLightningSpecials();
    assert(SamUltimatePolicy::Count()==28);
    assert(SamUltimatePolicy::UltimateCount()==11);
    assert(!SamUltimatePolicy::Moves[8].raiden && SamUltimatePolicy::Moves[8].release=="3014");
    assert(SamUltimatePolicy::Moves[9].playable && SamUltimatePolicy::Moves[9].release=="9108");
    assert(!SamUltimatePolicy::Moves[13].raiden && SamUltimatePolicy::Moves[13].release=="3004");
    SamUltimatePolicy::EnsureLightningSpecials();
    assert(SamUltimatePolicy::UltimateCount()==11);
    for(unsigned move=20;move<28;++move) {
        const auto s=Get(move);
        assert(Is(move) && s.pulses && s.targets);
        for(unsigned p=0;p<s.pulses;++p) {
            assert(PulseFrame(s,p,60)>0 && PulseFrame(s,p,60)<60);
            if(p) assert(PulseFrame(s,p,60)>PulseFrame(s,p-1,60));
        }
        assert(!Contains(s,0,4,1));
        assert(!Contains(s,100,0,100));
        assert(!Contains(s,std::numeric_limits<float>::quiet_NaN(),0,0));
        assert(Damage(100,1000,s.pulses)*int(s.pulses)<=60);
        assert(!SamUltimatePolicy::Moves[move].raiden);
        assert(!SamUltimatePolicy::Moves[move].windup.empty());
        for(const auto& code:SamUltimatePolicy::Moves[move].Stages()) {
            const bool playable=SamUltimatePolicy::Moves[move].playable;
            const std::string base=(playable ? "local_assets/data107/pl/pl1400.dat.unpacked/pl1400_" :
                "local_assets/data000/em/em0020.dat.unpacked/em0020_")+code;
            assert(std::ifstream(base+".mot",std::ios::binary).good());
            std::ifstream file(base+"_0_seq.bxm",std::ios::binary);
            if(!file.good()) {file.clear();file.open(base+"_2_seq.bxm",std::ios::binary);}
            std::vector<uint8_t> source((std::istreambuf_iterator<char>(file)),{});
            const auto original=source;
            auto adapted=SamBossSequence::Adapt(source.data(),source.size(),code.c_str(),false,playable);
            assert(!adapted.empty());
            const float contact=SamBossSequence::FirstHitFrame(source.data());
            if(code=="3004")assert(std::fabs(contact-8)<0.01f);
            if(code=="3024")assert(contact==1);
            if(code=="3014")assert(std::fabs(contact-14)<0.01f);
            SamBossSequence::DisableAttackTracks(adapted);
            const unsigned nodes=SamBossSequence::U16(adapted.data()+8);
            const unsigned ps=16+nodes*8,ts=ps+SamBossSequence::U16(adapted.data()+10)*4;
            for(unsigned n=0;n<nodes;++n) {
                const auto* node=adapted.data()+16+n*8;
                const unsigned ix=SamBossSequence::U16(node+6);
                const char* tag=reinterpret_cast<const char*>(adapted.data()+ts+SamBossSequence::U16(adapted.data()+ps+ix*4));
                if(!std::strcmp(tag,"AttackTrack")) assert(SamBossSequence::U16(node)==0);
            }
            assert(source==original);
        }
    }
    assert(Contains(Get(20),0,0,3));
    assert(!Contains(Get(20),0,0,5));
    assert(!Contains(Get(20),0,0,-1));
    assert(Contains(Get(22),0,0,-4));
    assert(Contains(Get(23),5,0,12));
    assert(!Contains(Get(25),1,0,12));
    assert(Contains(Get(25),0.5f,0,12));
    assert(Contains(Get(26),3,0,12));
    assert(Damage(0,100,1)==0 && Damage(100,0,1)==0);
    assert(!Is(19) && !Is(28));
    std::cout<<"PASS: lightning registration, legacy migration, targeting shapes, pulse timing and damage caps\n";
}
