#pragma once

#include "Protocol.hpp"
#include "tasks/TaskSerializer.hpp"

#include <vector>

/**
 * @brief      A synchronized fork
 *
 * We hold a flag indicating whether this is in use.
 * The access through the flag is always done in a serialized manner.
 */
class Fork {
public:
    Fork(int forkIdx, TaskExecutorPtr executor)
        : forkIdx_(forkIdx)
        , serializer_(executor) {}

    void request(int philosopherIdx, TaskExecutorPtr executor, Task onSuccess, Task onFailure) {
        serializer_.enqueue([this, philosopherIdx, executor, s = std::move(onSuccess),
                                    f = std::move(onFailure)] {
            if (!inUse_ || philosopherIdx_ == philosopherIdx) {
                inUse_ = true;
                philosopherIdx_ = philosopherIdx;
                executor->enqueue(s);
            } else
                executor->enqueue(f);
        });
    }
    void release() {
        serializer_.enqueue([this] {
            inUse_ = false;
        });
    }

private:
    //! The index of the fork
    int forkIdx_;
    //! Indicates if the fork is in used or not
    bool inUse_{false};
    //! The index of the philosopher using this fork; set only if inUse_
    int philosopherIdx_{0};
    //! The object used to serialize access to the fork
    TaskSerializer serializer_;
};

using ForkPtr = std::shared_ptr<Fork>;

class ForkLevelPhilosopherProtocol : public PhilosopherProtocol {
public:
    ForkLevelPhilosopherProtocol(
            int philosopherIdx, ForkPtr leftFork, ForkPtr rightFork, TaskExecutorPtr executor)
        : philosopherIdx_(philosopherIdx)
        , serializer_(std::make_shared<TaskSerializer>(executor))
        , executor_(executor) {
        forks_[0] = leftFork;
        forks_[1] = rightFork;
    }

    void startDining(Task eatTask, Task eatFailureTask, Task thinkTask, Task leaveTask) final {
        eatTask_ = std::move(eatTask);
        eatFailureTask_ = std::move(eatFailureTask);
        thinkTask_ = std::move(thinkTask);
        leaveTask_ = std::move(leaveTask);
        forksTaken_[0] = false;
        forksTaken_[1] = false;
        forksResponses_ = 0;
        executor_->enqueue(thinkTask); // Start by thinking
    }
    void onEatingDone(bool leavingTable) final {
        // Return the forks
        forks_[0]->release();
        forks_[1]->release();
        // Next action for the philosopher
        if (!leavingTable)
            executor_->enqueue(thinkTask_);
        else
            executor_->enqueue(leaveTask_);
    }
    void onThinkingDone() final {
        forks_[0]->request(philosopherIdx_, serializer_, [this] { onForkStatus(0, true); },
                [this] { onForkStatus(0, false); });
        forks_[1]->request(philosopherIdx_, serializer_, [this] { onForkStatus(1, true); },
                [this] { onForkStatus(1, false); });
    }

private:
    void onForkStatus(int forkIdx, bool isAcquired) {
        // Store the fork status
        forksTaken_[forkIdx] = isAcquired;

        // Do we have response from both forks?
        if (++forksResponses_ == 2) {
            if (forksTaken_[0] && forksTaken_[1]) {
                // Success
                executor_->enqueue(eatTask_);
            } else {
                // Release the forks
                if (forksTaken_[0]) {
                    forks_[0]->release();
                }
                if (forksTaken_[1]) {
                    forks_[1]->release();
                }
                // Philosopher just had an eating failure
                executor_->enqueue(eatFailureTask_);
            }
            // Reset this for the next round of eat request
            forksResponses_ = 0;
        }
    }

    //! The index of the philosopher
    int philosopherIdx_;
    //! The forks near the philosopher
    ForkPtr forks_[2];
    //! Indicates which forks are taken
    bool forksTaken_[2]{false, false};
    //! The number of responses got from the forks
    int forksResponses_{0};
    //! The executor of the tasks
    TaskExecutorPtr executor_;
    //! Serializer used to process notifications from the forks
    std::shared_ptr<TaskSerializer> serializer_;
    //! The implementation of the actions that the philosopher does
    Task eatTask_, eatFailureTask_, thinkTask_, leaveTask_;
};

class ForkLevelTableProtocol : public TableProtocol {
public:
    ForkLevelTableProtocol(int numSeats, TaskExecutorPtr executor)
        : executor_(executor) {
        // Create the forks
        forks_.reserve(numSeats);
        for (int i = 0; i < numSeats; i++)
            forks_.emplace_back(std::make_shared<Fork>(i, executor));
    }

    std::unique_ptr<PhilosopherProtocol> createPhilosopherProtocol(int idx) final {
        int numSeats = int(forks_.size());
        assert(idx >= 0);
        assert(idx < numSeats);
        ForkPtr leftFork = forks_[idx];
        ForkPtr rightFork = forks_[(idx + 1) % numSeats];
        return std::unique_ptr<PhilosopherProtocol>(
                new ForkLevelPhilosopherProtocol(idx, leftFork, rightFork, executor_));
    }

private:
    //! All the forks at the table
    std::vector<ForkPtr> forks_;
    //! The executor of the tasks
    TaskExecutorPtr executor_;
};
