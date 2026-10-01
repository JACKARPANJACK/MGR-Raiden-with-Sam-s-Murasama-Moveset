#include "../WeaponSwitchPolicy.h"
#include "../SamDlcRouting.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main()
{
    using namespace WeaponSwitchPolicy;
    assert(Cycle(0,-1)==Unarmed && Cycle(Unarmed,1)==Sword);
    assert(Weapons[Unarmed].unarmed && Weapons[Unarmed].object==0);
    assert(Weapons[Murasama].sam && !Weapons[Sword].sam);
    assert(Weapons[2].equipped==2 && Weapons[3].equipped==3 && Weapons[4].equipped==4);
    for(int i=0;i<Count;++i)
    {
        assert(InputSelection(i,true,false,0)==Cycle(i,-1)); // Q
        assert(InputSelection(i,false,true,0)==Cycle(i,1)); // E
        assert(InputSelection(i,false,false,1)==Cycle(i,-1)); // D-pad left
        assert(InputSelection(i,false,false,2)==Cycle(i,1)); // D-pad right
        assert(InputSelection(i,false,false,8)==Murasama);
        assert(InputSelection(i,false,false,4)==Sword);
        assert(InputSelection(i,true,true,0)==-1);
        assert(InputSelection(i,false,false,3)==-1);
    }
    assert(InputSelection(Sword,false,true,0)==Murasama);
    assert(InputSelection(Murasama,true,false,0)==Sword);
    assert(AcceptInput(true,true,false,false));
    assert(!AcceptInput(false,true,false,false));
    assert(!AcceptInput(true,false,false,false));
    assert(!AcceptInput(true,true,true,false));
    assert(!AcceptInput(true,true,false,true));
    assert(PadPress(2,2,0)==2);
    assert(PadPress(2,2,2)==0); // even repeated native trig cannot cycle a held key
    assert(PadPress(0,0,2)==0);
    assert(PadPress(2,2,0)==2); // release and re-press
    for(unsigned bit=0;bit<32;++bit)
        assert(((ReservedKeys>>bit)&1u)==unsigned(bit=='Q'%32 || bit=='E'%32));
    assert(CanSwitch(0,true,false,false,false,false));
    assert(CanSwitch(0x100000,true,false,false,false,false));
    assert(!CanSwitch(0x10000f,true,false,false,false,false));
    assert(!CanSwitch(0,true,false,true,false,false));
    assert(!CanSwitch(0,true,false,false,true,false));
    assert(!CanSwitch(0,true,false,false,false,true));
    assert(!CanSwitch(0,true,true,false,false,false));
    assert(!CanSwitch(0,false,false,false,false,false));
    char code[5]{};
    assert(!SamDlcRouting::Code(code,"8100",false,0x100007,true));
    assert(!SamDlcRouting::Code(code,"8100",true,0x47,false));
    unsigned checked=0, sharedChecked=0;
    const std::filesystem::path folder="local_assets/data107/pl/pl1400.dat.unpacked";
    for(const auto& entry:std::filesystem::directory_iterator(folder))
    {
        const auto name=entry.path().filename().string();
        if(name.rfind("pl1400_",0)!=0 || name.find("_2_seq.bxm")==std::string::npos) continue;
        const auto native=name.substr(7,4);
        if(native[0]!='8' && native[0]!='9' && native.rfind("210",0)!=0) continue;
        assert(SamDlcRouting::Code(code,name.c_str(),true,0x100007,true));
        assert(std::string(code)==native);
        auto clip=SamArchiveLookup::Resolve(code,SamArchiveLookup::Source::Playable,false,true,
            [&](SamArchiveLookup::Source,const char* file)->void* {
                return std::ifstream(folder/file,std::ios::binary).good()?reinterpret_cast<void*>(1):nullptr;
            });
        if(SamDlcRouting::SharedMotion(code))
        {
            assert(!std::ifstream(folder/("pl1400_"+native+".mot"),std::ios::binary).good());
            assert(std::ifstream(std::filesystem::path("local_assets/data000/pl/pl0010.dat.unpacked")/("pl0010_"+native+".mot"),std::ios::binary).good());
            ++sharedChecked;
        }
        else assert(clip.motion && clip.sequence && clip.source==SamArchiveLookup::Source::Playable);
        ++checked;
    }
    assert(checked>40);
    assert(sharedChecked==7);
    std::cout<<"PASS: six weapons, Q/E and D-pad selection, Sam enable/disable presets, switch guards, "<<checked-sharedChecked<<" DLC pairs and "<<sharedChecked<<" verified native shared execution motions with Sam sequences\n";
}
