#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace guandan::voice {

constexpr uint32_t kMagic = 0x4744564F;  ///< 魔数 'GDVO'
constexpr size_t kHeaderSize = 28;       ///< 包头字节长度
constexpr size_t kMaxPayload = 1024;     ///< 单包最大载荷

/// UDP 语音包类型
enum class PacketType : uint8_t {
    Join = 1,       ///< 加入房间
    Leave = 2,      ///< 离开房间
    Audio = 3,      ///< 音频数据
    Heartbeat = 4,  ///< 心跳
};

/// 网络字节序包头（大端）
struct PacketHeader {
    uint32_t magic = kMagic;       ///< 魔数
    uint8_t type = 0;              ///< PacketType
    uint8_t reserved[3] = {0, 0, 0};  ///< 保留
    uint32_t userId = 0;           ///< 发送者 userId
    uint32_t sequence = 0;         ///< 序号
    uint64_t timestamp = 0;        ///< 时间戳
    uint16_t payloadLen = 0;       ///< 载荷长度
    uint16_t reserved2 = 0;        ///< 保留
};

/// 主机 16 位转大端
inline uint16_t hostToBe16(uint16_t v) {
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

/// 主机 32 位转大端
inline uint32_t hostToBe32(uint32_t v) {
    return ((v & 0xFFu) << 24) | ((v & 0xFF00u) << 8) | ((v & 0xFF0000u) >> 8) |
           ((v & 0xFF000000u) >> 24);
}

/// 主机 64 位转大端
inline uint64_t hostToBe64(uint64_t v) {
    return ((v & 0xFFull) << 56) | ((v & 0xFF00ull) << 40) | ((v & 0xFF0000ull) << 24) |
           ((v & 0xFF000000ull) << 8) | ((v & 0xFF00000000ull) >> 8) |
           ((v & 0xFF0000000000ull) >> 24) | ((v & 0xFF000000000000ull) >> 40) |
           ((v & 0xFF00000000000000ull) >> 56);
}

/// 大端 16 位转主机
inline uint16_t beToHost16(uint16_t v) { return hostToBe16(v); }

/// 大端 32 位转主机
inline uint32_t beToHost32(uint32_t v) { return hostToBe32(v); }

/// 大端 64 位转主机
inline uint64_t beToHost64(uint64_t v) { return hostToBe64(v); }

/// 将包头写入 out（至少 kHeaderSize 字节）
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

/// 从 in 解析包头；len 不足或魔数/载荷非法返回 false
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
