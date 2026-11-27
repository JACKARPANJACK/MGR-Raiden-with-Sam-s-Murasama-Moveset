#include "../AnimationArchive.h"
#include "../StealthPoseFile.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

static std::vector<unsigned char> Read(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);assert(f);
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int main() {
    unsigned total=0;
    for(const auto& item: {std::make_pair("StealthAnims","pl1c00"),
                          std::make_pair("SamTemplateAnims","pl1c10"),
                          std::make_pair("BaseMoveset","pl1c20")}) {
        const auto packed=Read(std::string("Release/GameData/pl/")+item.second+".dat");
        AnimationArchive::Bank bank;
        assert(!bank.Load("GameData\\pl\\missing_animation_bank.dat"));
        const std::string archivePath=std::string("GameData\\pl\\")+item.second+".dat";
        assert(bank.Load(archivePath.c_str()));
        unsigned count=0;
        for(const auto& entry:AnimationArchiveNames::Members) {
            if(std::strcmp(entry.folder,item.first)!=0)continue;
            const auto member=bank.Member(entry.packed);
            assert(!member.empty());
            const auto source=std::filesystem::path("Release")/entry.folder/entry.source;
            assert(member==Read(source));++count;++total;
            if(source.extension()==".gwm") {
                StealthPoseFile::Clip pose;assert(pose.Load(member));
            }
        }
        assert(count==unsigned(std::strcmp(item.first,"StealthAnims")==0?82:
                               std::strcmp(item.first,"SamTemplateAnims")==0?28:2));
        assert(bank.Member("missing.mot").empty());
        auto broken=packed;broken[0]='X';assert(!bank.Parse(broken));
        assert(bank.Member("crouch_idle.gwm").empty());
        broken=packed;broken.resize(31);assert(!bank.Parse(broken));
        broken=packed;unsigned bad=0xffffffff;std::memcpy(broken.data()+8,&bad,4);assert(!bank.Parse(broken));
        broken=packed;unsigned names;std::memcpy(&names,broken.data()+16,4);
        std::memcpy(broken.data()+names,&bad,4);assert(!bank.Parse(broken));
        broken=packed;broken.pop_back();assert(!bank.Parse(broken));
        assert(bank.Parse(packed));
    }
    assert(total==112);
    std::cout<<"PASS: 112 packed members match source bytes, all 34 pose clips load, malformed DATs rejected.\n";
}
