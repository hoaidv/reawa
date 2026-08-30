#pragma once
/**
 * Opaque device-authored tree ids. UUID v4 mint is O(1).
 * Duplicate detection is insert-time via DeviceDocument's id→node map.
 * @implements [SRS-EP-07] unique tree ids
 * @implements [STORY-EP-067] Singleton generateNodeId for all tree nodes
 */

#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

namespace epaper {
namespace document {

inline std::string generateUuidV4()
{
    thread_local std::mt19937_64 rng{[] {
        std::random_device rd;
        std::seed_seq seed{rd(), rd(), rd(), rd()};
        return std::mt19937_64{seed};
    }()};
    const uint64_t hi = rng();
    const uint64_t lo = rng();
    const uint32_t timeLow = static_cast<uint32_t>(hi >> 32);
    const uint16_t timeMid = static_cast<uint16_t>(hi >> 16);
    const uint16_t timeHi = static_cast<uint16_t>((static_cast<uint16_t>(hi) & 0x0fffu) | 0x4000u);
    const uint16_t clock = static_cast<uint16_t>((static_cast<uint16_t>(lo >> 48) & 0x3fffu) | 0x8000u);
    const uint64_t node = lo & 0xffffffffffffull;
    char buf[37];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx", timeLow, timeMid, timeHi, clock,
                  static_cast<unsigned long long>(node));
    return std::string(buf, 36);
}

} // namespace document
} // namespace epaper
