#pragma once
#include <cstdint>
namespace SamPlayerPolicy {
inline constexpr std::uint32_t RaidenObject = 0x10010;
inline constexpr std::uint32_t GenosObject = 0x10f00;
constexpr bool Eligible(std::uint32_t behaviorObject, std::uint32_t entityObject, bool hasEntity) {
    return hasEntity && behaviorObject == RaidenObject && entityObject == RaidenObject;
}
}
