#pragma once

#include "Task.hpp"

#include <memory>

class TaskExecutor {
public:
    virtual ~TaskExecutor() {}
    virtual void enqueue(Task t) = 0;
};

using TaskExecutorPtr = std::shared_ptr<TaskExecutor>;
