#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace guandan::voice {

constexpr uint32_t kMagic = 0x4744564F;  // 'GDVO'
constexpr size_t kHeaderSize = 28;
constexpr size_t kMaxPayload = 1024;

enum class PacketType : uint8_t {
    Join = 1,
    Leave = 2,
    Audio = 3,
    Heartbeat = 4,
};

struct PacketHeader {
    uint32_t magic = kMagic;
    uint8_t type = 0;
    uint8_t reserved[3] = {0, 0, 0};
    uint32_t userId = 0;
    uint32_t sequence = 0;
    uint64_t timestamp = 0;
    uint16_t payloadLen = 0;
    uint16_t reserved2 = 0;
};

inline uint16_t hostToBe16(uint16_t v) {
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

inline uint32_t hostToBe32(uint32_t v) {
    return ((v & 0xFFu) << 24) | ((v & 0xFF00u) << 8) | ((v & 0xFF0000u) >> 8) |
           ((v & 0xFF000000u) >> 24);
}

inline uint64_t hostToBe64(uint64_t v) {
    return ((v & 0xFFull) << 56) | ((v & 0xFF00ull) << 40) | ((v & 0xFF0000ull) << 24) |
           ((v & 0xFF000000ull) << 8) | ((v & 0xFF00000000ull) >> 8) |
           ((v & 0xFF0000000000ull) >> 24) | ((v & 0xFF000000000000ull) >> 40) |
           ((v & 0xFF00000000000000ull) >> 56);
}

inline uint16_t beToHost16(uint16_t v) { return hostToBe16(v); }

inline uint32_t beToHost32(uint32_t v) { return hostToBe32(v); }

inline uint64_t beToHost64(uint64_t v) { return hostToBe64(v); }

inline void writeHeader(uint8_t* out, const PacketHeader& h) {
    auto* magic = reinterpret_cast<uint32_t*>(out);
    *magic = hostToBe32(h.magic);
    out[4] = h.type;
    out[5] = out[6] = out[7] = 0;
    auto* userId = reinterpret_cast<uint32_t*>(out + 8);
    *userId = hostToBe32(h.userId);
    auto* seq = reinterpret_cast<uint32_t*>(out + 12);
    *seq = hostToBe32(h.sequence);
    auto* ts = reinterpret_cast<uint64_t*>(out + 16);
    *ts = hostToBe64(h.timestamp);
    auto* len = reinterpret_cast<uint16_t*>(out + 24);
    *len = hostToBe16(h.payloadLen);
    out[26] = out[27] = 0;
}

inline bool readHeader(const uint8_t* in, size_t len, PacketHeader& h) {
    if (len < kHeaderSize) return false;
    h.magic = beToHost32(*reinterpret_cast<const uint32_t*>(in));
    if (h.magic != kMagic) return false;
    h.type = in[4];
    h.userId = beToHost32(*reinterpret_cast<const uint32_t*>(in + 8));
    h.sequence = beToHost32(*reinterpret_cast<const uint32_t*>(in + 12));
    h.timestamp = beToHost64(*reinterpret_cast<const uint64_t*>(in + 16));
    h.payloadLen = beToHost16(*reinterpret_cast<const uint16_t*>(in + 24));
    if (h.payloadLen > kMaxPayload) return false;
    return true;
}

}  // namespace guandan::voice
