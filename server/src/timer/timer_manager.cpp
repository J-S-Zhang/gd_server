#include "timer/timer_manager.h"

namespace guandan {

void TimerManager::schedule(const std::string& key, int seconds, TimerCallback callback) {
    std::lock_guard lock(mutex_);
    TimerEntry entry;
    entry.expireAt = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    entry.callback = std::move(callback);
    timers_[key] = std::move(entry);
}

void TimerManager::cancel(const std::string& key) {
    std::lock_guard lock(mutex_);
    timers_.erase(key);
}

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
