#include "../SamCombatRouting.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace SamCombatRouting;
    assert(IsNativeAirTransition("023a")); // Sam double jump was formerly filtered out.
    assert(IsNativeAirTransition("0232"));
    assert(IsNativeAirTransition("0243"));
    assert(!IsNativeAirTransition("0200"));
    assert(!IsNativeAirTransition(nullptr));
    assert(!IsNativeAirTransition("023a.mot"));
    assert(CodeFromFile("pl0000_2000.mot"));
    assert(CodeFromFile("pl0010_2000.mot"));
    assert(CodeFromFile("pl0010_2020_0_seq.bxm"));
    const char* acceptedRequests[] = {"2000", "2020", "24c1", "pl0010_2020", "pl0000_2000"};
    for (const char* name : acceptedRequests) assert(CodeFromRequest(name));
    const char* rejectedRequests[] = {nullptr, "", "200", "20000", "2000.mot",
        "9000", "0200", "pl1400_2000", "em0020_2000", "pl0010_2000.bak"};
    for (const char* name : rejectedRequests) assert(!CodeFromRequest(name));
    assert(CodeFromFile("pl0000_2020_0_seq.bxm"));
    assert(CodeFromFile("pl0000_2020_2_seq.bxm"));
    assert(IsMotion("pl0000_2000.mot"));
    assert(!IsMotion("pl0000_2000_0_seq.bxm"));
    const char* rejected[] = {nullptr, "", "pl0000_", "pl0000_2000",
        "pl0000_2000.mot.bak", "pl0000_2000_x_seq.bxm", "pl0000_9000.mot",
        "pl0000_0200.mot", "pl0000_0210.mot", "pl0000_4201.mot",
        "pl1400_2000.mot", "em0020_2000.mot"};
    for (const char* name : rejected)
        assert(!CodeFromFile(name));
    // Regression: these exact requests were recorded while the debug counter stayed zero.
    const char* liveRequests[] = {"2100", "2170", "2203", "2130", "212d", "2250",
        "2410", "2420", "2421", "2422", "2161", "2163", "2123", "2181",
        "2255", "2360", "2171", "2131", "2370", "2371", "2373", "2375",
        "2372", "2374", "2260", "2261", "2262", "2430", "2431", "2432",
        "2122", "2175", "220c"};
    for (const char* name : liveRequests) assert(CodeFromRequest(name));
    assert(std::strcmp(SamCodeForRaiden("2372"), "2211") == 0);
    assert(std::strcmp(SamCodeForRaiden("2374"), "2218") == 0);
    assert(std::strcmp(SamCodeForRaiden("2260"), "2020") == 0);
    assert(std::strcmp(SamCodeForRaiden("2430"), "2410") == 0);
    assert(std::strcmp(SamCodeForRaiden("2122"), "2001") == 0);
    assert(std::strcmp(SamCodeForRaiden("2175"), "2038") == 0);
    assert(std::strcmp(SamCodeForRaiden("220c"), "2120") == 0);
    assert(std::strcmp(SamCodeForRaiden("2121"), "2120") == 0);
    assert(std::strcmp(SamCodeForRaiden("2150"), "2410") == 0);
    assert(std::strcmp(SamCodeForRaiden("2320"), "2530") == 0);
    assert(std::strcmp(SamCodeForRaiden("2172"), "2600") == 0);
    assert(std::strcmp(SamCodeForRaiden("2601"), "2600") == 0);
    // Exercise the selection shared by the actual motion and sequence hooks.
    int samMotion = 1, samSequence = 2, nativeSequence = 3;
    RequestClip request;
    assert(request.Select("2000", [&](const char* code) {
        assert(std::strcmp(code, "2000") == 0);
        return SamArchiveLookup::Clip{&samMotion, &samSequence, SamArchiveLookup::Source::Playable};
    }));
    assert(request.clip.motion == &samMotion);
    assert(request.SequenceFor("pl0010_2000") == &samSequence);
    assert(request.SequenceFor("pl0000_2000") == &samSequence);
    assert(request.SequenceFor("2000") != &nativeSequence);
    assert(!request.SequenceFor("pl0010_2001"));
    assert(!request.SequenceFor("pl1400_2000"));
    assert(request.Select("2100", [&](const char* code) {
        assert(std::strcmp(code, "2000") == 0);
        return SamArchiveLookup::Clip{&samMotion, &samSequence, SamArchiveLookup::Source::Playable};
    }));
    assert(request.SequenceFor("pl0010_2100") == &samSequence);
    assert(!request.SequenceFor("pl0010_2000"));
    assert(!request.Select("2999", [&](const char*) { return SamArchiveLookup::Clip{}; }));
    assert(!request.clip.motion && !request.SequenceFor("pl0010_2999"));
    assert(!request.Select("2020", [&](const char*) {
        return SamArchiveLookup::Clip{&samMotion, nullptr, SamArchiveLookup::Source::Playable};
    }));
    assert(!request.SequenceFor("pl0010_2020"));
    assert(!request.SequenceFor("pl0010_2000"));
    assert(!request.Select("0200", [&](const char*) {
        assert(false); // locomotion must never reach the resolver
        return SamArchiveLookup::Clip{};
    }));
    assert(!request.clip.motion && !request.clip.sequence);
    std::cout << "PASS: combat file routing, sequence channels, native locomotion and actor isolation\n";
}
