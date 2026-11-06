#pragma once

#include "room/room_executor.h"
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

namespace guandan {

/// 单房间串行任务队列：保证同一房间消息顺序执行
class RoomTaskQueue : public std::enable_shared_from_this<RoomTaskQueue> {
public:
    using Task = std::function<void()>;  ///< 待执行任务

    explicit RoomTaskQueue(RoomExecutor& executor);

    /// 投递任务到本房间队列
    void post(Task task);

private:
    /// 将队列中的任务批量提交到 RoomExecutor
    void drain();

    RoomExecutor& executor_;       ///< 底层线程池执行器
    std::mutex mutex_;             ///< 保护 tasks_ 与 drainScheduled_
    std::deque<Task> tasks_;       ///< 待执行任务
    bool drainScheduled_{false};   ///< 是否已调度一次 drain
};

}  // namespace guandan
