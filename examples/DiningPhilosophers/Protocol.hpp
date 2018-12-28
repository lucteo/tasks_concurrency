#pragma once

#include "tasks/Task.hpp"

#include <memory>

/**
 * @brief      A protocol to be followed by a philosopher.
 *
 * This will be called by all philosopher to follow the 'house rules of the dinner'.
 * It will know to schedule thinking and eating activities.
 */
struct PhilosopherProtocol {
    virtual ~PhilosopherProtocol() {}

    //! Called when a philosopher joins the dining table.
    //! Describes the eating, thinking and leaving behavior of the philosopher.
    virtual void startDining(Task eatTask, Task eatFailureTask, Task thinkTask, Task leaveTask) = 0;

    //! Called when a philosopher is done eating.
    //! The given flag indicates if the philosopher had all the meals and its leaving
    virtual void onEatingDone(bool leavingTable) = 0;
    //! Called when a philosopher is done thinking.
    virtual void onThinkingDone() = 0;
};

/**
 * @brief      The house rules for the dinner
 *
 * This will create PhilosopherProtocol objects for each of the philosophers joining the dinner.
 * This way, the protocols given to each individual will be coherent.
 */
struct TableProtocol {
    virtual ~TableProtocol() {}

    //! Called to create an individual protocol to pass to a philosopher.
    virtual std::unique_ptr<PhilosopherProtocol> createPhilosopherProtocol(int idx) = 0;
};

