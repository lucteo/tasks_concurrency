#pragma once

#include "TaskExecutor.hpp"

#include "tbb/concurrent_queue.h"
#include "tbb/atomic.h"

class TaskSerializer : public TaskExecutor {
public:
    TaskSerializer(TaskExecutorPtr executor);

    void enqueue(Task t) override;

private:
    //! The base executor we are using for executing the tasks passed to the serializer
    TaskExecutorPtr baseExecutor_;
    //! Queue of tasks that are not yet in execution
    tbb::concurrent_queue<Task> standbyTasks_;
    //! Indicates the number of tasks in the standby queue
    tbb::atomic<int> count_{0};

    //! Enqueues for execution the first task in our standby queue
    void enqueueFirst();

    //! Called when we finished executing one task, to continue with other tasks
    void onTaskDone();
};
