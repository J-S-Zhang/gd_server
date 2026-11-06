#include "timer/timer_manager.h"
#include <vector>

/// @file timer_manager.cpp
/// @brief 定时器注册与 tick 驱动

namespace guandan {

/// 实现 schedule 相关逻辑
void TimerManager::schedule(const std::string& key, int seconds, TimerCallback callback) {
    std::lock_guard lock(mutex_);
    TimerEntry entry;
    entry.expireAt = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    entry.callback = std::move(callback);
    timers_[key] = std::move(entry);
}

/// 取消解散投票
void TimerManager::cancel(const std::string& key) {
    std::lock_guard lock(mutex_);
    timers_.erase(key);
}

/// 驱动定时器到期回调
void TimerManager::tick() {
    std::vector<TimerCallback> expired;
    {
        std::lock_guard lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        for (auto it = timers_.begin(); it != timers_.end(); ) {
            if (it->second.expireAt <= now) {
                expired.push_back(it->second.callback);
                it = timers_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& cb : expired) {
        if (cb) cb();
    }
}

}  // namespace guandan
