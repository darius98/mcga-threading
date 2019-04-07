#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

namespace mcga::threading::base {

template<class Processor>
class DelayedQueueWrapper {
 private:
    using Clock = std::chrono::steady_clock;
 public:
    using Task = typename Processor::Task;
    using Delay = std::chrono::nanoseconds;
    class DelayedTask;
    using DelayedTaskPtr = std::shared_ptr<DelayedTask>;
    class DelayedTask {
     public:
        bool isCancelled() const {
            return cancelled;
        }

        bool cancel() {
            return cancelled.exchange(true);
        }

        bool isInterval() const {
            return isRepeated;
        }

     private:
        class MakeSharedEnabler : public DelayedTask {
         public:
            MakeSharedEnabler(Task task, const Delay& delay, bool isRepeated)
                    : DelayedTask(std::move(task), delay, isRepeated) {}
        };

        struct Compare {
            inline bool operator()(
                    const DelayedTaskPtr& a, const DelayedTaskPtr& b) const {
                return a->timePoint > b->timePoint;
            }
        };

        static DelayedTaskPtr delayed(Task task, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(
                    std::move(task), delay, false);
        }

        static DelayedTaskPtr interval(Task task, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(
                    std::move(task), delay, true);
        }

        DelayedTask(Task task, const Delay& delay, bool isRepeated):
                task(std::move(task)), delay(delay), isRepeated(isRepeated) {
            setTimePoint();
        }

        bool shouldExecute() const {
            return timePoint <= Clock::now();
        }

        void setTimePoint() {
            timePoint = Clock::now()
                        + std::chrono::duration_cast<Clock::duration>(delay);
        }

        Task task;
        Delay delay;
        Clock::time_point timePoint;
        bool isRepeated;
        std::atomic_bool cancelled = false;

     friend class DelayedQueueWrapper;
    };

 public:
    DelayedTaskPtr enqueueDelayed(const Task& task, const Delay& delay) {
        return enqueueDelayedTask(DelayedTask::delayed(task, delay));
    }

    DelayedTaskPtr enqueueDelayed(Task&& task, const Delay& delay) {
        return enqueueDelayedTask(DelayedTask::delayed(std::move(task), delay));
    }

    DelayedTaskPtr enqueueInterval(const Task& task, const Delay& delay) {
        return enqueueDelayedTask(DelayedTask::interval(task, delay));
    }

    DelayedTaskPtr enqueueInterval(Task&& task, const Delay& delay) {
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
