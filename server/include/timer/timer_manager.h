#pragma once

#include "game/types.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace guandan {

using TimerCallback = std::function<void()>;  ///< 定时器到期回调

/// 按键管理的秒级定时器（需在主循环调用 tick）
class TimerManager {
public:
    /// 在 key 下调度 seconds 秒后执行 callback（同 key 会覆盖）
    void schedule(const std::string& key, int seconds, TimerCallback callback);
    /// 取消 key 对应定时器
    void cancel(const std::string& key);
    /// 检查到期并触发回调
    void tick();

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point expireAt;  ///< 到期时间
        TimerCallback callback;                          ///< 到期回调
    };

    std::mutex mutex_;                                      ///< 保护 timers_
    std::unordered_map<std::string, TimerEntry> timers_;    ///< 定时器表
};

}  // namespace guandan
