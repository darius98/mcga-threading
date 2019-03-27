#pragma once

#include <mutex>
#include <queue>
#include <thread>

#include "delayed_invocation.hpp"
#include "executable.hpp"

namespace mcga::threading {

class EventLoop {
 public:
    EventLoop() = default;

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
        return enqueue(DelayedInvocation::Delayed(
                func,
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::Delayed(
                std::move(func),
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::Interval(
                func,
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return enqueue(DelayedInvocation::Interval(
                std::move(func),
                std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

 private:
    DelayedInvocationPtr enqueue(DelayedInvocationPtr invocation);

    void executePending();

    std::size_t getImmediateQueueSize() const;
    std::size_t getDelayedQueueSize() const;

    DelayedInvocationPtr popDelayedQueue();
    Executable popImmediateQueue();

    std::atomic_bool running = false;

    mutable std::mutex immediateQueueLock;
    std::queue<Executable> immediateQueue;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedInvocationPtr,
                        std::vector<DelayedInvocationPtr>,
                        DelayedInvocation::Compare> delayedQueue;

};

}
