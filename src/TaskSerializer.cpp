#include "tasks/TaskSerializer.hpp"

TaskSerializer::TaskSerializer(TaskExecutorPtr executor)
    : baseExecutor_(std::move(executor)) {}

void TaskSerializer::enqueue(Task t) {
    // Add the task to our standby queue
    standbyTasks_.emplace(std::move(t));

    // If this the first task in our standby queue, start executing it
    if (++count_ == 1)
        enqueueFirst();
}

void TaskSerializer::enqueueFirst() {
    // Get the task to execute
    Task toExecute;
    bool res = standbyTasks_.try_pop(toExecute);
    assert(res);
    // Enqueue the task
    baseExecutor_->enqueue([this, t = std::move(toExecute)] {
        // Execute current task
        t();
        // Check for continuation
        this->onTaskDone();
    });
}

void TaskSerializer::onTaskDone() {
    // If we still have tasks in our standby queue, enqueue the first one.
    // One at a time.
    if (--count_ != 0)
        enqueueFirst();
}
