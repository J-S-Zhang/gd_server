#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace guandan {

enum class EmotionType : uint8_t {
    Flower = 1,
    Heart = 2,
    Like = 3,
};

bool isValidEmotionType(int value);

class EmotionRateLimiter {
public:
    bool tryAcquire(uint64_t playerId);
    void remove(uint64_t playerId);

private:
    std::mutex mutex_;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> lastSendTime_;
};

}  // namespace guandan
