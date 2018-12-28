#pragma once

#include "TaskExecutor.hpp"

class GlobalTaskExecutor : public TaskExecutor {
public:
    GlobalTaskExecutor();

    void enqueue(Task t) override;
};
