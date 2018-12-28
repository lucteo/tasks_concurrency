#pragma once

#include "Protocol.hpp"
#include "tasks/TaskSerializer.hpp"

#include <vector>

/**
 * @brief      Waiter that hands the forks to the philosophers.
 *
 * The waiter is needed to ensure a synchronization point between philosophers.
 * Each philosopher will request the forks to the waiter. The access to the waiter is serialized.
 * That is, only one philosopher can talk to the waiter at a given time.
 */
class Waiter {
public:
    Waiter(int numSeats, TaskExecutorPtr executor)
        : executor_(executor)
        , serializer_(executor) {
        // Arrange the forks on the table; they are not in use at this time
        forksInUse_.resize(numSeats, false);
    }

    void requestForks(int philosopherIdx, Task onSuccess, Task onFailure) {
        serializer_.enqueue([this, philosopherIdx, onSuccess = std::move(onSuccess),
                                    onFailure = std::move(onFailure)] {
            this->doRequestForks(philosopherIdx, std::move(onSuccess), std::move(onFailure));
        });
    }

    void returnForks(int philosopherIdx) {
        serializer_.enqueue([this, philosopherIdx] { this->doReturnForks(philosopherIdx); });
    }

private:
    //! Called when a philosopher requests the forks for eating.
    //! If the forks are available, mark them as being in use and execute the onSucceess task.
    //! If the forks are not available, execute the onFailure task.
    //! This is always called under our serializer.
    void doRequestForks(int philosopherIdx, Task onSuccess, Task onFailure) {
        int numSeats = forksInUse_.size();
        int idxLeft = philosopherIdx;
        int idxRight = (philosopherIdx + 1) % numSeats;

        if (!forksInUse_[idxLeft] && !forksInUse_[idxRight]) {
            executor_->enqueue(std::move(onSuccess)); // enqueue asap
            forksInUse_[idxLeft] = true;
            forksInUse_[idxRight] = true;
        } else {
            executor_->enqueue(std::move(onFailure));
        }
    }

    //! Called when a philosopher is done eating and returns the forks
    void doReturnForks(int philosopherIdx) {
        int numSeats = forksInUse_.size();
        int idxLeft = philosopherIdx;
        int idxRight = (philosopherIdx + 1) % numSeats;
        assert(forksInUse_[idxLeft]);
        assert(forksInUse_[idxRight]);
        forksInUse_[idxLeft] = false;
        forksInUse_[idxRight] = false;
    }

    //! The forks on the table, with flag indicating whether they are in use or not
    std::vector<bool> forksInUse_;
    //! The executor used to schedule tasks
    TaskExecutorPtr executor_;
    //! Serializer object used to ensure serialized accessed to the waiter
    TaskSerializer serializer_;
};

class WaiterPhilosopherProtocol : public PhilosopherProtocol {
public:
    WaiterPhilosopherProtocol(
            int philosopherIdx, std::shared_ptr<Waiter> waiter, TaskExecutorPtr executor)
        : philosopherIdx_(philosopherIdx)
        , waiter_(waiter)
        , executor_(executor) {}

    void startDining(Task eatTask, Task eatFailureTask, Task thinkTask, Task leaveTask) final {
        eatTask_ = std::move(eatTask);
        eatFailureTask_ = std::move(eatFailureTask);
        thinkTask_ = std::move(thinkTask);
        leaveTask_ = std::move(leaveTask);
        executor_->enqueue(thinkTask);  // Start by thinking
    }
    void onEatingDone(bool leavingTable) final {
        // Return the forks
        waiter_->returnForks(philosopherIdx_);
        // Next action for the philosopher
        if (!leavingTable)
            executor_->enqueue(thinkTask_);
        else
            executor_->enqueue(leaveTask_);
    }
    void onThinkingDone() final {
        waiter_->requestForks(philosopherIdx_, eatTask_, eatFailureTask_);
    }

private:
    //! The index of the philosopher
    int philosopherIdx_;
    //! The waiter who is responsible for handling and receiving the forks
    std::shared_ptr<Waiter> waiter_;
    //! The executor of the tasks
    TaskExecutorPtr executor_;
    //! The implementation of the actions that the philosopher does
    Task eatTask_, eatFailureTask_, thinkTask_, leaveTask_;
};

class WaiterTableProtocol : public TableProtocol {
public:
    WaiterTableProtocol(int numSeats, TaskExecutorPtr executor)
        : waiter_(std::make_shared<Waiter>(numSeats, executor))
        , executor_(executor) {}

    std::unique_ptr<PhilosopherProtocol> createPhilosopherProtocol(int idx) final {
        return std::unique_ptr<PhilosopherProtocol>(
                new WaiterPhilosopherProtocol(idx, waiter_, executor_));
    }

private:
    //! The waiter who is responsible for handling and receiving the forks
    std::shared_ptr<Waiter> waiter_;
    //! The executor of the tasks
    TaskExecutorPtr executor_;
};
