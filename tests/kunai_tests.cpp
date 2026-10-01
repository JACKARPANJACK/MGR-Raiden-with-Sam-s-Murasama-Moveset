#include "../KunaiPolicy.h"
#include <array>
#include <cassert>
#include <filesystem>
#include <iostream>
int main()
{
    using namespace KunaiPolicy;
    assert(Cycle(Native,-1)==Heat && Cycle(Heat,1)==Native);
    for(unsigned state:{0u,1u,4u,0x100000u,0x100002u})
    {
        assert(CanSelect(state,true,false,false,false));
        assert(!CanSelect(state,true,false,true,false));
        assert(!CanSelect(state,true,true,false,false));
        assert(!CanSelect(state,true,false,false,true));
        assert(!CanSelect(state,false,false,false,false));
    }
    for(unsigned state:{9u,0x50u,0x100012u,0x100040u,0x500u}) assert(!CanSelect(state,true,false,false,false));
    assert(CanSuspend(0x100000,true,false,false));
    assert(!CanSuspend(0x100012,true,false,false));
    assert(!CanSuspend(0x100000,true,true,false));
    assert(!CanResume(0,true,true,90)); // never restore Sam while aim is held
    assert(!CanResume(0,true,false,2)); // give native graph time to enter aim
    assert(!CanResume(0x50,true,false,30)); // grenade recovery must complete
    assert(CanResume(0,true,false,30));
    for(int variant=Stun;variant<Count;++variant)
    {
        std::array<unsigned char,0x320> original{},shot{};
        for(unsigned i=0;i<original.size();++i) original[i]=static_cast<unsigned char>(i*7+3);
        shot=original;Configure(shot.data(),variant);
        unsigned object=0;std::memcpy(&object,shot.data()+4,4);assert(object==Object);
        // Preserve friendly collision flags, owner pointer/handle, all native
        // aiming vectors, target attachment and grenade inventory metadata.
        for(unsigned i=0;i<shot.size();++i)
        {
            const bool changed=i<8 || (i>=0x10 && i<0x20) || (i>=0x110 && i<0x118) || (i>=0x160 && i<0x168);
            if(!changed) assert(shot[i]==original[i]);
        }
        assert(Matches(Attack,ImpactDamage(variant),variant,true));
        assert(!Matches(Attack,ImpactDamage(variant),variant,false));
        assert(!Matches(0x57,ImpactDamage(variant),variant,true));
        assert(ImpactDamage(variant)<=20);
        assert(!TimedDetonation(variant,1,true));
    }
    assert(TimedDetonation(Explosive,3,true));
    assert(TimedDetonation(Explosive,180,false));
    assert(!TimedDetonation(Stun,180,true));assert(!TimedDetonation(Heat,180,true));
    assert(BurnDamage(50)==1 && BurnDamage(100000)==3);
    for(const char* name:{"wp0372.wmb","wp0372.eff","wp0372_col.hkx"})
        assert(std::filesystem::file_size(std::string("local_assets/data000/wp/wp0372.dat.unpacked/")+name)>0);
    std::cout<<"Kunai native descriptor preservation, selection, Sam bridge, collision ownership and damage bounds passed.\n";
}
