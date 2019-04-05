#pragma once

#include <queue>
#include <thread>
#include <vector>

#include <concurrentqueue.h>

#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/loop_tick_duration.hpp>

namespace mcga::threading::constructs {

template<class Exec>
class EventLoopConstruct : private Exec {
 public:
    using Object = typename Exec::Object;

    using Delay = std::chrono::nanoseconds;
    using Clock = std::chrono::steady_clock;

    class DelayedInvocation;
    using DelayedInvocationPtr = std::shared_ptr<DelayedInvocation>;
    class DelayedInvocation {
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
        class MakeSharedEnabler : public DelayedInvocation {
         public:
            MakeSharedEnabler(Object obj, const Delay& delay, bool isRepeated)
                    : DelayedInvocation(std::move(obj), delay, isRepeated) {}
        };

        struct Compare {
            inline bool operator()(
                    const std::shared_ptr<DelayedInvocation>& a,
                    const std::shared_ptr<DelayedInvocation>& b) const {
                return a->timePoint > b->timePoint;
            }
        };

        static DelayedInvocationPtr delayed(Object obj, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(
                    std::move(obj), delay, false);
        }

        static DelayedInvocationPtr interval(Object obj, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(
                    std::move(obj), delay, true);
        }

        DelayedInvocation(Object obj, const Delay& delay, bool isRepeated):
                obj(std::move(obj)), delay(delay), isRepeated(isRepeated) {
            setTimePoint();
        }

        bool shouldExecute() const {
            return timePoint <= Clock::now();
        }

        void setTimePoint() {
            timePoint = Clock::now()
                        + std::chrono::duration_cast<Clock::duration>(delay);
        }

        Object obj;
        Delay delay;
        Clock::time_point timePoint;
        bool isRepeated;
        std::atomic_bool cancelled = false;

     friend class EventLoopConstruct;
    };

    using Exec::Exec;

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

    void enqueue(const Object& obj) {
        immediateQueue.enqueue(obj);
    }

    void enqueue(Object&& obj) {
        immediateQueue.enqueue(std::move(obj));
    }

    DelayedInvocationPtr enqueueDelayed(
            const Object& obj, const Delay& delay) {
        return enqueue(DelayedInvocation::delayed(obj, delay));
    }

    DelayedInvocationPtr enqueueDelayed(
            Object&& obj, const Delay& delay) {
        return enqueue(DelayedInvocation::delayed(std::move(obj), delay));
    }

    DelayedInvocationPtr enqueueInterval(
            const Object& obj, const Delay& delay) {
        return enqueue(DelayedInvocation::interval(obj, delay));
    }
    DelayedInvocationPtr enqueueInterval(
            Object&& obj, const Delay& delay) {
        return enqueue(DelayedInvocation::interval(std::move(obj), delay));
    }

 private:
    DelayedInvocationPtr enqueue(DelayedInvocationPtr invocation) {
        std::lock_guard guard(delayedQueueLock);
        delayedQueue.push(invocation);
        return invocation;
    }

    void run() {
        auto delayedInvocation = popDelayedQueue();
        if (delayedInvocation != nullptr) {
            if (!delayedInvocation->isCancelled()) {
                this->handleObject(delayedInvocation->obj);
            }
            if (!delayedInvocation->isCancelled()
                && delayedInvocation->isInterval()) {
                delayedInvocation->setTimePoint();
                enqueue(delayedInvocation);
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
                    this->handleObject(immediateQueueBuffer[i]);
                }
            }
        }
    }

    std::size_t getDelayedQueueSize() const {
        std::lock_guard guard(delayedQueueLock);
        return delayedQueue.size();
    }

    DelayedInvocationPtr popDelayedQueue() {
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

    moodycamel::ConcurrentQueue<Object> immediateQueue;
    moodycamel::ConsumerToken immediateQueueToken{immediateQueue};
    std::vector<Object> immediateQueueBuffer;
    std::size_t numImmediateDequeued = 0;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedInvocationPtr,
                        std::vector<DelayedInvocationPtr>,
                        typename DelayedInvocation::Compare> delayedQueue;
};

}  // namespace mcga::threading::constructs
