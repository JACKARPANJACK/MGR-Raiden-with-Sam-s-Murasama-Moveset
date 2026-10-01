#pragma once
#include <cstdio>
#include <cstring>

// Animation codes overlap. Keep motion and sequence in the SAME archive.
namespace SamArchiveLookup
{
    enum class Source { Playable, Boss };
    struct Clip
    {
        void* motion = nullptr;
        void* sequence = nullptr;
        Source source = Source::Playable;
    };

    inline bool ValidCode(const char* code)
    {
        if (!code || std::strlen(code) != 4) return false;
        for (int i = 0; i < 4; ++i)
            if (!((code[i] >= '0' && code[i] <= '9') ||
                  (code[i] >= 'a' && code[i] <= 'f'))) return false;
        return true;
    }

    template<class Lookup>
    Clip Resolve(const char* code, Source preferred, bool allowFallback,
                 bool requireSequence, Lookup lookup)
    {
        if (!ValidCode(code)) return {};
        for (int attempt = 0; attempt < (allowFallback ? 2 : 1); ++attempt)
        {
            const Source source = attempt == 0 ? preferred :
                (preferred == Source::Playable ? Source::Boss : Source::Playable);
            const char* prefix = source == Source::Playable ? "pl1400" : "em0020";
            char filename[64]{};
            std::snprintf(filename, sizeof(filename), "%s_%s.mot", prefix, code);
            Clip clip{lookup(source, filename), nullptr, source};
            if (!clip.motion) continue;
            const int channels[3] = {source == Source::Playable ? 2 : 0,
                                     source == Source::Playable ? 0 : 2, 3};
            for (int channel : channels)
            {
                std::snprintf(filename, sizeof(filename), "%s_%s_%d_seq.bxm", prefix, code, channel);
                clip.sequence = lookup(source, filename);
                if (clip.sequence) break;
            }
            if (clip.sequence || !requireSequence) return clip;
        }
        return {};
    }
}
