#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    using VoiceSocketAddr = sockaddr_in;
#else
    #include <netinet/in.h>
    using VoiceSocketAddr = sockaddr_in;
#endif

namespace guandan::voice {

/// UDP 语音客户端登记信息
struct VoiceClient {
    uint32_t userId = 0;                              ///< 用户 id
    std::string roomId;                                 ///< 所在语音房间（游戏 roomId）
    VoiceSocketAddr addr {};                            ///< UDP 地址
    std::chrono::steady_clock::time_point lastSeen {};  ///< 最后活跃时间
};

/// 按房间维护语音 peer 列表，用于转发音频包
class VoiceRoomManager {
public:
    /// 加入语音房间并登记地址
    void join(uint32_t userId, const std::string& roomId, const VoiceSocketAddr& addr);
    /// 离开语音
    void leave(uint32_t userId);
    /// 更新地址与心跳时间
    void touch(uint32_t userId, const VoiceSocketAddr& addr);
    /// 同房间除自己外的 peer 地址列表
    std::vector<VoiceSocketAddr> peersExcept(uint32_t userId, const std::string& roomId) const;
    /// 查询用户当前语音 roomId
    bool roomIdForUser(uint32_t userId, std::string& roomIdOut) const;
    /// 移除长时间无心跳的客户端
    void pruneIdle(std::chrono::seconds maxIdle);

private:
    mutable std::mutex mutex_;                                      ///< 保护 clients_ 与 roomMembers_
    std::unordered_map<uint32_t, VoiceClient> clients_;             ///< userId -> 客户端
    std::unordered_map<std::string, std::vector<uint32_t>> roomMembers_;  ///< roomId -> 成员 userId 列表
};

}  // namespace guandan::voice
