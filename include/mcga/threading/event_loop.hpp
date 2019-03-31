#pragma once

#include <mutex>
#include <queue>
#include <thread>

#include <concurrentqueue.h>

#include "internal/delayed_invocation.hpp"
#include "internal/executable.hpp"
#include "internal/disallow_copy_and_move.hpp"

namespace mcga::threading {

class EventLoop {
 public:
    EventLoop();
    ~EventLoop() = default;

    DISALLOW_COPY_AND_MOVE(EventLoop);

    std::size_t size() const;

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

}  // namespace mcga::threading
