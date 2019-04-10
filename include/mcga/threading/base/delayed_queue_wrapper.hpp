#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include <mcga/threading/base/delayed_task.hpp>
#include <mcga/threading/base/execute_task_variants.hpp>

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

    template<class Enqueuer>
    bool executeDelayed(Processor* processor, Enqueuer* enqueuer) {
        auto delayedTask = this->popDelayedQueue();
        if (delayedTask == nullptr) {
            return false;
        }
        if (!delayedTask->isCancelled()) {
            executeTask(std::move(delayedTask->task), processor, enqueuer);
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
