#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "delayed_task.hpp"

namespace mcga::threading::base {

template<class Processor>
class DelayedQueueWrapper {
 public:
    using Task = typename Processor::Task;
 private:
    using DelayedTask = DelayedTask<Task>;
    using Clock = typename DelayedTask::Clock;
 public:
    using DelayedTaskPtr = typename DelayedTask::DelayedTaskPtr;
    using Delay = std::chrono::nanoseconds;

    DelayedTaskPtr enqueueDelayed(Task task, const Delay& delay) {
        return enqueueDelayedTask(DelayedTask::delayed(std::move(task), delay));
    }

    DelayedTaskPtr enqueueInterval(Task task, const Delay& delay) {
        return enqueueDelayedTask(
                DelayedTask::interval(std::move(task), delay));
    }

 protected:
    DelayedTaskPtr enqueueDelayedTask(DelayedTaskPtr delayedTask) {
        std::lock_guard guard(queueLock);
        queue.push(delayedTask);
        return delayedTask;
    }

    std::size_t getDelayedQueueSize() const {
        std::lock_guard guard(queueLock);
        return queue.size();
    }

    DelayedTaskPtr popDelayedQueue() {
        std::lock_guard guard(queueLock);
        if (queue.empty()) {
            return nullptr;
        }
        auto top = queue.top();
        if (!top->shouldExecute()) {
            return nullptr;
        }
        queue.pop();
        return top;
    }

    bool executeDelayed(Processor* processor) {
        auto delayedTask = this->popDelayedQueue();
        if (delayedTask == nullptr) {
            return false;
        }
        if (!delayedTask->isCancelled()) {
            if (delayedTask->isInterval()) {
                processor->executeTask(delayedTask->task);
            } else {
                processor->executeTask(std::move(delayedTask->task));
            }
        }
        if (!delayedTask->isCancelled() && delayedTask->isInterval()) {
            delayedTask->setTimePoint();
            this->enqueueDelayedTask(delayedTask);
        }
        return true;
    }

 private:
    mutable std::mutex queueLock;
    std::priority_queue<DelayedTaskPtr,
                        std::vector<DelayedTaskPtr>,
                        typename DelayedTask::Compare> queue;
};

}  // namespace mcga::threading::base
