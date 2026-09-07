#pragma once

#include "game/types.h"
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace guandan {

class RoomTaskQueue {
public:
    using Task = std::function<void()>;

    void post(Task task);
    void processAll();

private:
    std::mutex mutex_;
    std::queue<Task> tasks_;
};

}  // namespace guandan
