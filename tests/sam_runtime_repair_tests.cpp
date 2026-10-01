#include "../SamStagePolicy.h"
#include "../SamBalancePolicy.h"
#include "../SamBossSequence.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <iterator>
#include <iostream>
static std::vector<unsigned char> Read(const char* path)
{
    std::ifstream f(path,std::ios::binary);
    return {(std::istreambuf_iterator<char>(f)),{}};
}
int main()
{
    for (const char* code : {"3210","3506","a649","3017"})
    {
        std::string base=std::string("local_assets/data000/em/em0020.dat.unpacked/em0020_")+code;
        auto motion=Read((base+".mot").c_str()); assert(motion.size()>12);
        unsigned duration=SamStagePolicy::Length(motion.data()); assert(duration>20);
        assert(!SamStagePolicy::Finished(4,duration)); // stale native end flag cannot end a stage
        assert(SamStagePolicy::Release(20,20));
        assert(!SamStagePolicy::Finished(20,duration)); // impact fires before completion
        assert(SamStagePolicy::Finished(float(duration),duration));
        auto seq=Read((base+"_0_seq.bxm").c_str());
        auto adapted=SamBossSequence::Adapt(seq.data(),seq.size(),code);
        assert(!adapted.empty()); assert(SamBossSequence::HitCount(adapted.data())>=1);
    }
    auto sam=Read("local_assets/data107/pl/pl1400.dat.unpacked/pl1400_battleParameter.bin");
    assert(sam.size()>4); unsigned rows=*reinterpret_cast<const unsigned*>(sam.data());
    assert(sam.size()==4+rows*76); // native table consists of 76-byte units
    bool light=false,heavy=false,charge=false;
    for (unsigned i=0;i<rows;++i)
    {
        const auto* row=sam.data()+4+i*76;
        int no=*reinterpret_cast<const int*>(row+44), power=*reinterpret_cast<const int*>(row+4);
        if (no==4) {assert(power>0 && power<100); light=true;}
        if (no==12) {assert(power>0 && power<100); heavy=true;}
        if (no==26) {assert(power>0 && power<100); charge=true;}
    }
    assert(light && heavy && charge);
    using namespace SamBalancePolicy;
    assert(Damage(-1,4,false,1,100)==0);
    assert(Damage(100000,4,false,1,100)<=3);
    assert(Damage(100000,26,false,1,100)<=8);
    int rapid=Damage(100000,4,true,20,500);
    assert(rapid*20<=100); // entire imported rapid stage <=20% target max HP
    assert(ProjectileDamage(100000,500)*8<=80); // complete flight <=16%
    assert(AttackSpeed==1.20f && RunSpeed==1.20f);
    std::cout << "PASS: real clip durations/impact ordering, genuine Sam attack table, normal/charged/rapid and Round Trip damage budgets\n";
}
