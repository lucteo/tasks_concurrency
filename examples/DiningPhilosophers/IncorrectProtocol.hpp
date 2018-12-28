#pragma once

#include "Protocol.hpp"
#include "tasks/TaskExecutor.hpp"

//! Incorrect protocol. Just schedules tasks.
class IncorrectPhilosopherProtocol : public PhilosopherProtocol {
public:
    IncorrectPhilosopherProtocol(TaskExecutorPtr executor)
        : executor_(executor) {}

    void startDining(Task eatTask, Task eatFailureTask, Task thinkTask, Task leaveTask) final {
        eatTask_ = std::move(eatTask);
        eatFailureTask_ = std::move(eatFailureTask);
        thinkTask_ = std::move(thinkTask);
        leaveTask_ = std::move(leaveTask);
        executor_->enqueue(thinkTask);  // Start by thinking
    }
    void onEatingDone(bool leavingTable) final {
        if (!leavingTable)
            executor_->enqueue(thinkTask_);
        else
            executor_->enqueue(leaveTask_);
    }
    void onThinkingDone() final {
        executor_->enqueue(eatTask_);
    }

private:
    //! The executor of the tasks
    TaskExecutorPtr executor_;
    //! The implementation of the actions that the philosopher does
    Task eatTask_, eatFailureTask_, thinkTask_, leaveTask_;
};

//! Table protocol that creates IncorrectPhilosopherProtocol objects for philosophers
class IncorrectTableProtocol : public TableProtocol {
public:
    IncorrectTableProtocol(TaskExecutorPtr executor)
        : executor_(executor) {}

    std::unique_ptr<PhilosopherProtocol> createPhilosopherProtocol(int /*idx*/) final {
        return std::unique_ptr<PhilosopherProtocol>(new IncorrectPhilosopherProtocol(executor_));
    }

private:
    //! The executor of the tasks
    TaskExecutorPtr executor_;
};
