#pragma once

#include "Protocol.hpp"
#include "Utils.hpp"

#include "tbb/atomic.h"

#include <string>
#include <memory>

/**
 * @brief      Represents a philosopher at the dinner.
 *
 * This will know how to eat and think, but will follow a given protocol when eating/thinking.
 * The eating and the thinking will be treated as tasks.
 * We make a distinction between pure thinking and thinking when failing to eat.
 * A philosopher will only eat a given number of time until it's considered "done".
 *
 * @see PhilosopherProtocol
 */
class Philosopher {
public:
    Philosopher(const char* name)
        : name_(name)
        , eventLog_(name) {}

    //! Called when the philosopher joins the dinner.
    //! It follows the protocol to consume the given number of meals.
    void start(std::unique_ptr<PhilosopherProtocol> protocol, int numMeals) {
        mealsRemaining_ = numMeals;
        doneDining_ = false;
        protocol_ = std::move(protocol);

        //! Describe ourselves to the protocol, and start dining
        auto eatTask = [this] { this->doEat(); };
        auto eatFailureTask = [this] { this->doEatFailure(); };
        auto thinkTask = [this] { this->doThink(); };
        auto leaveTask = [this] { this->doLeave(); };
        protocol_->startDining(eatTask, eatFailureTask, thinkTask, leaveTask);
    }

    //! Checks if the philosopher is done with the dinner
    bool isDone() const { return doneDining_; }

    //! Getter for the event log of the philosopher
    //! WARNING: Not thread-safe!
    const PhilosopherEventLog& eventLog() const { return eventLog_; }

private:
    //! The body of the eating task for the philosopher
    void doEat() {
        eventLog_.startActivity(ActivityType::eat);
        wait(randBetween(10, 50));
        eventLog_.endActivity(ActivityType::eat);

        // According to the protocol, announce the end of eating
        bool leavingTable = mealsRemaining_ > 1;
        protocol_->onEatingDone(--mealsRemaining_ == 0);
    }
    //! The body of the eating task for the philosopher
    void doEatFailure() {
        eventLog_.startActivity(ActivityType::eatFailure);
        wait(randBetween(5, 10));
        eventLog_.endActivity(ActivityType::eatFailure);

        protocol_->onThinkingDone();
    }
    //! The body of the thinking task for the philosopher
    void doThink() {
        eventLog_.startActivity(ActivityType::think);
        wait(randBetween(5, 30));
        eventLog_.endActivity(ActivityType::think);

        protocol_->onThinkingDone();
    }
    //! The body of the leaving task for the philosopher
    void doLeave() {
        eventLog_.startActivity(ActivityType::leave);
        doneDining_ = true;
    }

    //! The name of the philosopher.
    std::string name_;
    //! The number of meals remaining for the philosopher as part of the dinner.
    int mealsRemaining_{0};
    //! True if the philosopher is done dining and left the table
    tbb::atomic<bool> doneDining_{false};
    //! The protocol to follow at the dinner.
    std::unique_ptr<PhilosopherProtocol> protocol_;
    //! The event log for this philosopher
    PhilosopherEventLog eventLog_;
};
