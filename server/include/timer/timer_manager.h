#pragma once

#include "game/types.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace guandan {

using TimerCallback = std::function<void()>;

class TimerManager {
public:
    void schedule(const std::string& key, int seconds, TimerCallback callback);
    void cancel(const std::string& key);
    void tick();

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point expireAt;
        TimerCallback callback;
    };

    std::mutex mutex_;
    std::unordered_map<std::string, TimerEntry> timers_;
};

}  // namespace guandan
