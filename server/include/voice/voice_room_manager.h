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

struct VoiceClient {
    uint32_t userId = 0;
    std::string roomId;
    VoiceSocketAddr addr {};
    std::chrono::steady_clock::time_point lastSeen {};
};

class VoiceRoomManager {
public:
    void join(uint32_t userId, const std::string& roomId, const VoiceSocketAddr& addr);
    void leave(uint32_t userId);
    void touch(uint32_t userId, const VoiceSocketAddr& addr);
    std::vector<VoiceSocketAddr> peersExcept(uint32_t userId, const std::string& roomId) const;
    bool roomIdForUser(uint32_t userId, std::string& roomIdOut) const;
    void pruneIdle(std::chrono::seconds maxIdle);

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, VoiceClient> clients_;
    std::unordered_map<std::string, std::vector<uint32_t>> roomMembers_;
};

}  // namespace guandan::voice
