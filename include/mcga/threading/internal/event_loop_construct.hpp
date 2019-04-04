#pragma once

#include <queue>

#include <concurrentqueue.h>

#include "disallow_copy_and_move.hpp"
#include "loop_tick_duration.hpp"
#include "thread_pool_wrapper.hpp"
#include "thread_wrapper.hpp"

namespace mcga::threading::internal {

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
                    : DelayedInvocation(move(obj), delay, isRepeated) {}
        };

        struct Compare {
            inline bool operator()(
                    const std::shared_ptr<DelayedInvocation>& a,
                    const std::shared_ptr<DelayedInvocation>& b) const {
                return a->timePoint > b->timePoint;
            }
        };

        static DelayedInvocationPtr delayed(Object obj, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(move(obj), delay, false);
        }

        static DelayedInvocationPtr interval(Object obj, const Delay& delay) {
            return std::make_shared<MakeSharedEnabler>(move(obj), delay, true);
        }

        DelayedInvocation(Object obj, const Delay& delay, bool isRepeated):
                obj(move(obj)), delay(delay), isRepeated(isRepeated) {
            setTimePoint();
        }

        bool shouldExecute() const {
            return timePoint <= Clock::now();
        }

        void executeIfNotCancelled() {
            if (!cancelled) {
                obj();
            }
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

    EventLoopConstruct(): immediateQueueToken(immediateQueue) {}

    DISALLOW_COPY_AND_MOVE(EventLoopConstruct);

    ~EventLoopConstruct() = default;

    std::size_t sizeApprox() const {
        return immediateQueue.size_approx()
               + getDelayedQueueSize()
               + numImmediateDequeued;
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            run();
            std::this_thread::sleep_for(loopTickDuration);
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
            delayedInvocation->executeIfNotCancelled();
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
    moodycamel::ConsumerToken immediateQueueToken;
    std::vector<Object> immediateQueueBuffer;
    std::atomic_size_t numImmediateDequeued = 0;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedInvocationPtr,
                        std::vector<DelayedInvocationPtr>,
                        typename DelayedInvocation::Compare> delayedQueue;
};

template<class W>
class EventLoopThreadConstruct : public ThreadWrapper<W> {
 public:
    using Object = typename W::Object;
    using DelayedInvocation = typename W::DelayedInvocation;
    using DelayedInvocationPtr = typename W::DelayedInvocationPtr;

    EventLoopThreadConstruct() = default;

    DISALLOW_COPY_AND_MOVE(EventLoopThreadConstruct);

    ~EventLoopThreadConstruct() = default;

    void enqueue(const Object& obj) {
        this->worker.enqueue(obj);
    }

    void enqueue(Object&& obj) {
        this->worker.enqueue(std::move(obj));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            const Object& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->worker.enqueueDelayed(obj, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Object&& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->worker.enqueueDelayed(std::move(obj), delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Object& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->worker.enqueueInterval(obj, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Object&& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->worker.enqueueInterval(std::move(obj), delay);
    }

 private:
    explicit EventLoopThreadConstruct(volatile std::atomic_bool* started):
            ThreadWrapper<W>(started) {}

 friend class ThreadPoolWrapper<EventLoopThreadConstruct>;
};

template<class W>
class EventLoopThreadPoolConstruct : public ThreadPoolWrapper<W> {
 public:
    using Object = typename W::Object;
    using DelayedInvocation = typename W::DelayedInvocation;
    using DelayedInvocationPtr = typename W::DelayedInvocationPtr;

    using ThreadPoolWrapper<W>::ThreadPoolWrapper;

    DISALLOW_COPY_AND_MOVE(EventLoopThreadPoolConstruct);

    ~EventLoopThreadPoolConstruct() = default;

    void enqueue(const Object& func) {
        this->nextThread()->enqueue(func);
    }

    void enqueue(Object&& func) {
        this->nextThread()->enqueue(std::move(func));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            const Object& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->nextThread()->enqueueDelayed(obj, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Object&& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->nextThread()->enqueueDelayed(std::move(obj), delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Object& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->nextThread()->enqueueInterval(obj, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Object&& obj,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return this->nextThread()->enqueueInterval(std::move(obj), delay);
    }
};

}  // namespace mcga::threading::internal
