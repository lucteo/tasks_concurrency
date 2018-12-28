#include "tasks/GlobalTaskExecutor.hpp"

#include "tbb/task.h"

//! Wrapper to transform our Task into a TBB task
struct TaskWrapper : tbb::task {
    Task ftor_;

    TaskWrapper(Task t)
        : ftor_(std::move(t)) {}

    tbb::task* execute() {
        ftor_();
        return nullptr;
    }
};

GlobalTaskExecutor::GlobalTaskExecutor() {}

void GlobalTaskExecutor::enqueue(Task t) {
    auto& tbbTask = *new (tbb::task::allocate_root()) TaskWrapper(std::move(t));
    tbb::task::enqueue(tbbTask);
}