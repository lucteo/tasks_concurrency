
#include "Utils.hpp"
#include "Protocol.hpp"
#include "Philosopher.hpp"
#include "IncorrectProtocol.hpp"
#include "WaiterProtocol.hpp"
#include "WaiterFairProtocol.hpp"
#include "ForkLevelProtocol.hpp"
#include "tasks/GlobalTaskExecutor.hpp"

#include <vector>
#include "tbb/task_scheduler_init.h"

const char* philosopherNames[] = {"Socrates", "Plato", "Aristotle", "Descartes", "Spinoza", "Kant",
        "Schopenhauer", "Nietzsche", "Wittgenstein", "Heidegger", "Sartre"};
static constexpr int numPhilosophers = 3;
// static constexpr int numPhilosophers = sizeof(philosopherNames) / sizeof(philosopherNames[0]);

void organizeDinner(TableProtocol& tableProtocol) {
    // Create all the philosophers objects
    std::vector<Philosopher> philosophers;
    philosophers.reserve(numPhilosophers);
    for (int i = 0; i < numPhilosophers; i++) {
        philosophers.emplace_back(philosopherNames[i]);
    }

    // Start the dinner. At start, each philosopher will think
    constexpr int numMeals = 3;
    for (int i = 0; i < numPhilosophers; i++)
        philosophers[i].start(tableProtocol.createPhilosopherProtocol(i), numMeals);

    // Wait until every philosopher leaves the dinner
    // Use poor's man synchronization
    bool isDone = true;
    do {
        isDone = true;
        wait(50);
        for (const auto& ph : philosophers)
            isDone = isDone && ph.isDone();
    } while (!isDone);

    // Now print the event logs for all the philosophers
    printf("\n");
    for (const auto& ph : philosophers)
        ph.eventLog().printSummary();
}

int main(int /*argc*/, char** /*argv*/) {

    // Ensure we have enough worker threads
    tbb::task_scheduler_init init(numPhilosophers + 1);

    TaskExecutorPtr globalExecutor = std::make_shared<GlobalTaskExecutor>();

    IncorrectTableProtocol incorrectTableProtocol{globalExecutor};
    WaiterTableProtocol waiterTableProtocol{numPhilosophers, globalExecutor};
    WaiterFairTableProtocol waiterFairTableProtocol{numPhilosophers, globalExecutor};
    ForkLevelTableProtocol forkLevelTableProtocol{numPhilosophers, globalExecutor};

    // organizeDinner(incorrectTableProtocol);
    // organizeDinner(waiterTableProtocol);
    // organizeDinner(waiterFairTableProtocol);
    organizeDinner(forkLevelTableProtocol);

    return 0;
}