#include "../SamBossSequence.h"
#include "../SamUltimatePolicy.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>

std::vector<uint8_t> Read(const std::filesystem::path& path)
{
    std::ifstream f(path,std::ios::binary);
    return {(std::istreambuf_iterator<char>(f)),{}};
}
void Save(const std::filesystem::path& path,const std::vector<uint8_t>& bytes)
{
    std::ofstream f(path,std::ios::binary);
    f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    assert(f.good());
}
int main()
{
    namespace fs=std::filesystem;
    const fs::path output="Release/effect-sequences";
    fs::create_directories(output);
    std::set<std::string> codes;
    for (const auto& move:SamUltimatePolicy::Moves)
        for (const char* code:{move.windup,move.release}) if(code) codes.insert(code);
    for (const auto& entry:fs::directory_iterator("local_assets/data000/em/em0020.dat.unpacked"))
    {
        const std::string name=entry.path().filename().string();
        if (name.size()<18 || name.find("seq.bxm")==std::string::npos || !codes.count(name.substr(7,4))) continue;
        const auto original=Read(entry.path());
        auto source=original;
        auto result=SamBossSequence::Adapt(source.data(),source.size(),name.substr(7,4).c_str());
        assert(!result.empty() && source==original);
        Save(output/name,result);
    }
    const fs::path recovery="local_assets/data107/pl/pl1400.dat.unpacked/pl1400_242b_2_seq.bxm";
    auto source=Read(recovery), original=source;
    auto result=SamBossSequence::RepairPlayableEffects(source.data(),source.size());
    assert(!result.empty() && original==source);
    assert(SamBossSequence::RepairPlayableEffects(source.data(),source.size()-1).empty());
    Save(output/recovery.filename(),result);
    std::cout<<"PASS: exported actual runtime effect adaptations for asset/timing verification\n";
}
