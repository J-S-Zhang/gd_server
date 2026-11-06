#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace guandan {

/// 房间级任务线程池：多房间任务并发，单房间通过 RoomTaskQueue 串行
class RoomExecutor {
public:
    explicit RoomExecutor(std::size_t threadCount = 4);
    ~RoomExecutor();

    RoomExecutor(const RoomExecutor&) = delete;
    RoomExecutor& operator=(const RoomExecutor&) = delete;

    /// 投递异步任务
    void post(std::function<void()> task);

private:
    /// 工作线程主循环
    void workerLoop();

    std::vector<std::thread> workers_;              ///< 工作线程
    std::mutex mutex_;                              ///< 保护 tasks_ 与 stop_
    std::condition_variable cv_;                    ///< 任务到达/停止通知
    std::deque<std::function<void()>> tasks_;       ///< 全局任务队列
    bool stop_{false};                                ///< 停止标志
};

}  // namespace guandan
