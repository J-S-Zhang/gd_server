#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace guandan {

/// 房间内互动表情类型
enum class EmotionType : uint8_t {
    Flower = 1,       ///< 鲜花
    Tomato = 2,       ///< 番茄
    BoxingGlove = 3,  ///< 拳套
};

/// 校验整数值是否为合法表情类型
bool isValidEmotionType(int value);

/// 表情发送频率限制（按玩家）
class EmotionRateLimiter {
public:
    /// 尝试获取发送许可，成功返回 true
    bool tryAcquire(uint64_t playerId);
    /// 玩家离开房间时移除记录
    void remove(uint64_t playerId);

private:
    std::mutex mutex_;  ///< 保护 lastSendTime_
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> lastSendTime_;  ///< 上次发送时间
};

}  // namespace guandan
