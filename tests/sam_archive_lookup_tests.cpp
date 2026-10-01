#include "../SamArchiveLookup.h"
#include <cassert>
#include <map>
#include <string>
#include <iostream>

int main()
{
    using namespace SamArchiveLookup;
    int pm = 1, ps = 2, bm = 3, bs = 4;
    std::map<std::pair<Source, std::string>, void*> files;
    auto lookup = [&](Source source, const char* name) -> void* {
        auto it = files.find({source, name});
        return it == files.end() ? nullptr : it->second;
    };
    files[{Source::Playable, "pl1400_2000.mot"}] = &pm;
    files[{Source::Boss, "em0020_2000.mot"}] = &bm;
    files[{Source::Boss, "em0020_2000_0_seq.bxm"}] = &bs;
    auto clip = Resolve("2000", Source::Playable, true, true, lookup);
    assert(clip.motion == &bm && clip.sequence == &bs && clip.source == Source::Boss);
    assert(!Resolve("2000", Source::Playable, false, true, lookup).motion);
    clip = Resolve("2000", Source::Playable, false, false, lookup);
    assert(clip.motion == &pm && !clip.sequence);
    files[{Source::Playable, "pl1400_2000_2_seq.bxm"}] = &ps;
    clip = Resolve("2000", Source::Playable, true, true, lookup);
    assert(clip.motion == &pm && clip.sequence == &ps);
    clip = Resolve("2000", Source::Boss, false, true, lookup);
    assert(clip.motion == &bm && clip.sequence == &bs);
    files.erase({Source::Boss, "em0020_2000_0_seq.bxm"});
    assert(!Resolve("2000", Source::Boss, false, true, lookup).motion);
    files[{Source::Boss, "em0020_2000_3_seq.bxm"}] = &bs;
    assert(Resolve("2000", Source::Boss, false, true, lookup).sequence == &bs);
    const char* invalidCodes[] = {nullptr, "", "200", "20000", "../x", "zzzz"};
    for (const char* invalid : invalidCodes)
        assert(!Resolve(invalid, Source::Playable, true, true, lookup).motion);
    assert(!Resolve("ffff", Source::Playable, true, true, lookup).motion);
    std::cout << "PASS: archive identity, coherent fallback, explicit boss selection, sequence channels, missing/invalid clips\n";
}
