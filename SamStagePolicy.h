#pragma once
#include <cstdint>
namespace SamStagePolicy
{
    inline unsigned Length(const void* motion)
    {
        if (!motion) return 0;
        const auto* p=static_cast<const uint8_t*>(motion);
        if (p[0]!='m' || p[1]!='o' || p[2]!='t' || p[3]!=0) return 0;
        unsigned frames=unsigned(p[10]) | (unsigned(p[11])<<8);
        return frames && frames<=3600 ? frames : 0;
    }
    inline bool Finished(float frame, unsigned length)
    { return length > 0 && frame >= float(length); }
    inline bool Release(float frame, unsigned release)
    { return frame >= float(release); }
}
