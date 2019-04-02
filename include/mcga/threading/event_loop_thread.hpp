#pragma once

#include <mutex>
#include <queue>
#include <thread>

#include <concurrentqueue.h>

#include "internal/delayed_invocation.hpp"
#include "internal/thread_wrapper.hpp"

namespace mcga::threading {

class EventLoop {
 public:
    using Executable = DelayedInvocation::Executable;

    EventLoop();
    ~EventLoop() = default;

    DISALLOW_COPY_AND_MOVE(EventLoop);

    std::size_t sizeApprox() const;

    bool isRunning() const;
    void start();
    void stop();

    void enqueue(const Executable& func);
    void enqueue(Executable&& func);

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::delayed(
                func,
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::delayed(
                std::move(func),
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::interval(
                func,
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::interval(
                std::move(func),
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

 private:
    DelayedInvocationPtr enqueue(DelayedInvocationPtr invocation);

    void executePending();

    std::size_t getDelayedQueueSize() const;
    DelayedInvocationPtr popDelayedQueue();

    std::atomic_bool running = false;

    moodycamel::ConcurrentQueue<Executable> immediateQueue;
    moodycamel::ConsumerToken immediateQueueToken;
    std::vector<Executable> immediateQueueBuffer;
    std::atomic_size_t numImmediateDequeued = 0;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedInvocationPtr,
                        std::vector<DelayedInvocationPtr>,
                        DelayedInvocation::Compare> delayedQueue;

};

class EventLoopThread: public internal::ThreadWrapper<EventLoop> {
 public:
    using Executable = EventLoop::Executable;

    EventLoopThread() = default;
    ~EventLoopThread() = default;

    DISALLOW_COPY_AND_MOVE(EventLoopThread);

    void enqueue(const Executable& func);
    void enqueue(Executable&& func);

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return worker.enqueueDelayed(func, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return worker.enqueueDelayed(std::move(func), delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return worker.enqueueInterval(func, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return worker.enqueueInterval(std::move(func), delay);
    }
};

}  // namespace mcga::threading
