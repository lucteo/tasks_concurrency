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
 *
 * This Waiter implements a fair policy. It keeps a waiting lists of the philosophers that requested
 * the forks and did not get them. If one requests the forks, and one of the neighbors are still
 * waiting for the forks, then deny the request.
 */
class WaiterFair {
public:
    WaiterFair(int numSeats, TaskExecutorPtr executor)
        : executor_(executor)
        , serializer_(executor) {
        // Arrange the forks on the table; they are not in use at this time
        forksInUse_.resize(numSeats, false);
        // The waiting queue is bounded by the number of seats
        waitingQueue_.reserve(numSeats);
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
    //! If the forks are available, and there is nobody before in the waiting queue, accept the fork
    //! request. Otherwise, deny it, and make sure that the philosopher is in the waiting queue.
    void doRequestForks(int philosopherIdx, Task onSuccess, Task onFailure) {
        int numSeats = forksInUse_.size();
        int idxLeft = philosopherIdx;
        int idxRight = (philosopherIdx + 1) % numSeats;

        bool canEat = !forksInUse_[idxLeft] && !forksInUse_[idxRight];
        if ( canEat ) {
            // Ensure that the neighbors are not in the waiting queue before this one
            for ( int i=0; i<int(waitingQueue_.size()); i++) {
                int phId = waitingQueue_[i];
                if (phId == philosopherIdx) {
                    // We found ourselves in the list, before any of the neighbors.
                    // Remove ourselves from the waiting list and approve the eating.
                    waitingQueue_.erase(waitingQueue_.begin() + i);
                    break;
                }
                else if (phId == idxLeft || phId == idxRight) {
                    // A neighbor is before us in the waiting list.
                    // Deny the eating.
                    canEat = false;
                    break;
                }
            }
        }

        if (canEat) {
            // Mark the forks as being in use, and start eating
            executor_->enqueue(std::move(onSuccess)); // enqueue asap
            forksInUse_[idxLeft] = true;
            forksInUse_[idxRight] = true;

            // The philosopher should not be in the waiting queue anymore
            assert(std::find(waitingQueue_.begin(), waitingQueue_.end(), philosopherIdx) ==
                    waitingQueue_.end());
        } else {
            executor_->enqueue(std::move(onFailure)); // enqueue asap

            // Ensure that this philosopher is on the list; if it's not, add it
            auto it = std::find(waitingQueue_.begin(), waitingQueue_.end(), philosopherIdx);
            if (it == waitingQueue_.end())
                waitingQueue_.push_back(philosopherIdx);
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
    //! The eat requests that are failed, in order
    std::vector<int> waitingQueue_;
    //! The executor used to schedule tasks
    TaskExecutorPtr executor_;
    //! Serializer object used to ensure serialized accessed to the waiter
    TaskSerializer serializer_;
};

class WaiterFairPhilosopherProtocol : public PhilosopherProtocol {
public:
    WaiterFairPhilosopherProtocol(
            int philosopherIdx, std::shared_ptr<WaiterFair> waiter, TaskExecutorPtr executor)
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
    std::shared_ptr<WaiterFair> waiter_;
    //! The executor of the tasks
    TaskExecutorPtr executor_;
    //! The implementation of the actions that the philosopher does
    Task eatTask_, eatFailureTask_, thinkTask_, leaveTask_;
};

class WaiterFairTableProtocol : public TableProtocol {
public:
    WaiterFairTableProtocol(int numSeats, TaskExecutorPtr executor)
        : waiter_(std::make_shared<WaiterFair>(numSeats, executor))
        , executor_(executor) {}

    std::unique_ptr<PhilosopherProtocol> createPhilosopherProtocol(int idx) final {
        return std::unique_ptr<PhilosopherProtocol>(
                new WaiterFairPhilosopherProtocol(idx, waiter_, executor_));
    }

private:
    //! The waiter who is responsible for handling and receiving the forks
    std::shared_ptr<WaiterFair> waiter_;
    //! The executor of the tasks
    TaskExecutorPtr executor_;
};
