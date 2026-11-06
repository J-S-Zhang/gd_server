#include "room/room_executor.h"

/// @file room_executor.cpp
/// @brief 房间业务线程池

namespace guandan {

/// 构造 RoomExecutor 实例
RoomExecutor::RoomExecutor(std::size_t threadCount) {
    if (threadCount < 1) threadCount = 1;
    workers_.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

/// 析构并释放资源
RoomExecutor::~RoomExecutor() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

/// 投递异步任务
void RoomExecutor::post(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        tasks_.push_back(std::move(task));
    }
    cv_.notify_one();
}

/// 实现 workerLoop 相关逻辑
void RoomExecutor::workerLoop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        if (task) task();
    }
}

}  // namespace guandan
