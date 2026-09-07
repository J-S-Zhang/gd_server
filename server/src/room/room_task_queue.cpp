#include "room/room_task_queue.h"

namespace guandan {

void RoomTaskQueue::post(Task task) {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
}

void RoomTaskQueue::processAll() {
    std::queue<Task> local;
    {
        std::lock_guard lock(mutex_);
        std::swap(local, tasks_);
    }
    while (!local.empty()) {
        local.front()();
        local.pop();
    }
}

}  // namespace guandan
