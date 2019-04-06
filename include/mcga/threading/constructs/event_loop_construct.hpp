#pragma once

#include <queue>
#include <thread>
#include <vector>

#include <concurrentqueue.h>

#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/loop_tick_duration.hpp>

namespace mcga::threading::constructs {

template<class Processor>
class EventLoopConstruct : private Processor {
 public:
    using Task = typename Processor::Task;

    using Delay = std::chrono::nanoseconds;
    using Clock = std::chrono::steady_clock;

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

     friend class EventLoopConstruct;
    };

    using Processor::Processor;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopConstruct);

    ~EventLoopConstruct() = default;

    std::size_t sizeApprox() const {
        return immediateQueue.size_approx()
               + getDelayedQueueSize()
               + numImmediateDequeued;
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            run();
            std::this_thread::sleep_for(base::loopTickDuration);
        }
    }

    void enqueue(const Task& task) {
        immediateQueue.enqueue(task);
    }

    void enqueue(Task&& task) {
        immediateQueue.enqueue(std::move(task));
    }

    DelayedTaskPtr enqueueDelayed(const Task& task, const Delay& delay) {
        return enqueue(DelayedTask::delayed(task, delay));
    }

    DelayedTaskPtr enqueueDelayed(Task&& task, const Delay& delay) {
        return enqueue(DelayedTask::delayed(std::move(task), delay));
    }

    DelayedTaskPtr enqueueInterval(const Task& task, const Delay& delay) {
        return enqueue(DelayedTask::interval(task, delay));
    }
    DelayedTaskPtr enqueueInterval(Task&& task, const Delay& delay) {
        return enqueue(DelayedTask::interval(std::move(task), delay));
    }

 private:
    DelayedTaskPtr enqueue(const DelayedTaskPtr& delayedTask) {
        std::lock_guard guard(delayedQueueLock);
        delayedQueue.push(delayedTask);
        return delayedTask;
    }

    void run() {
        auto delayedTask = popDelayedQueue();
        if (delayedTask != nullptr) {
            if (!delayedTask->isCancelled()) {
                this->executeTask(delayedTask->task);
            }
            if (!delayedTask->isCancelled() && delayedTask->isInterval()) {
                delayedTask->setTimePoint();
                enqueue(delayedTask);
            }
        } else {
            auto immediateQueueSize = immediateQueue.size_approx();
            if (immediateQueueSize > 0) {
                if (immediateQueueSize > immediateQueueBuffer.size()) {
                    immediateQueueBuffer.resize(immediateQueueSize);
                }
                numImmediateDequeued = immediateQueue.try_dequeue_bulk(
                        immediateQueueToken,
                        immediateQueueBuffer.begin(),
                        immediateQueueBuffer.size());
                for (size_t i = 0;
                     numImmediateDequeued > 0;
                     --numImmediateDequeued, ++ i) {
                    this->executeTask(immediateQueueBuffer[i]);
                }
            }
        }
    }

    std::size_t getDelayedQueueSize() const {
        std::lock_guard guard(delayedQueueLock);
        return delayedQueue.size();
    }

    DelayedTaskPtr popDelayedQueue() {
        std::lock_guard guard(delayedQueueLock);
        if (delayedQueue.empty()) {
            return nullptr;
        }
        auto top = delayedQueue.top();
        if (!top->shouldExecute()) {
            return nullptr;
        }
        delayedQueue.pop();
        return top;
    }

    moodycamel::ConcurrentQueue<Task> immediateQueue;
    moodycamel::ConsumerToken immediateQueueToken{immediateQueue};
    std::vector<Task> immediateQueueBuffer;
    std::size_t numImmediateDequeued = 0;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedTaskPtr,
                        std::vector<DelayedTaskPtr>,
                        typename DelayedTask::Compare> delayedQueue;
};

}  // namespace mcga::threading::constructs
