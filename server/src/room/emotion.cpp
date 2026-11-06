#include "room/emotion.h"

/// @file emotion.cpp
/// @brief 表情发送频率限制

namespace guandan {

bool isValidEmotionType(int value) {
    switch (static_cast<EmotionType>(value)) {
        case EmotionType::Flower:
        case EmotionType::Tomato:
        case EmotionType::BoxingGlove:
            return true;
        default:
            return false;
    }
}

/// 实现 tryAcquire 相关逻辑
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

/// 实现 remove 相关逻辑
void EmotionRateLimiter::remove(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    lastSendTime_.erase(playerId);
}

}  // namespace guandan
