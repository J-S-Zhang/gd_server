#include "room/room_task_queue.h"
#include "utils/logger.h"
#include <exception>

/// @file room_task_queue.cpp
/// @brief 房间级任务队列投递与排空

namespace guandan {

/// 构造 RoomTaskQueue 实例
RoomTaskQueue::RoomTaskQueue(RoomExecutor& executor) : executor_(executor) {}

/// 投递异步任务
void RoomTaskQueue::post(Task task) {
    bool needSchedule = false;
    {
        std::lock_guard lock(mutex_);
        tasks_.push_back(std::move(task));
        if (!drainScheduled_) {
            drainScheduled_ = true;
            needSchedule = true;
        }
    }
    if (needSchedule) {
        auto self = shared_from_this();
        executor_.post([self] { self->drain(); });
    }
}

/// 实现 drain 相关逻辑
void RoomTaskQueue::drain() {
    for (;;) {
        Task task;
        {
            std::lock_guard lock(mutex_);
            if (tasks_.empty()) {
                drainScheduled_ = false;
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        try {
            if (task) task();
        } catch (const std::exception& e) {
            Logger::error(std::string("RoomTaskQueue task failed: ") + e.what());
        } catch (...) {
            Logger::error("RoomTaskQueue task failed: unknown exception");
        }
    }
}

}  // namespace guandan
