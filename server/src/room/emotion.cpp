#include "room/emotion.h"

namespace guandan {

bool isValidEmotionType(int value) {
    switch (static_cast<EmotionType>(value)) {
        case EmotionType::Flower:
        case EmotionType::Heart:
        case EmotionType::Like:
            return true;
        default:
            return false;
    }
}

bool EmotionRateLimiter::tryAcquire(uint64_t playerId) {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = lastSendTime_.find(playerId);
    if (it != lastSendTime_.end() &&
        now - it->second < std::chrono::milliseconds(1000)) {
        return false;
    }

    lastSendTime_[playerId] = now;
    return true;
}

void EmotionRateLimiter::remove(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    lastSendTime_.erase(playerId);
}

}  // namespace guandan
