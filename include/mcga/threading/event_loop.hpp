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

    template<class D>
    explicit EventLoop(D tick) {
        setTick(tick);
    }

    template<class D>
    void setTick(D _tick) {
        tick = std::chrono::duration_cast<std::chrono::nanoseconds>(_tick);
    }

    std::size_t getNumPendingJobs() const {
        std::scoped_lock guard(immediateQueueLock, delayedQueueLock);
        return immediateQueue.size() + delayedQueue.size();
    }

    void start() {
        running = true;
        while (running) {
            executePending();
            std::this_thread::sleep_for(tick);
        }
    }

    void stop() {
        running = false;
    }

    void enqueue(const Executable& func) {
        std::lock_guard guard(immediateQueueLock);
        immediateQueue.push(func);
    }

    void enqueue(Executable&& func) {
        std::lock_guard guard(immediateQueueLock);
        immediateQueue.push(std::move(func));
    }

    template<class D>
    DelayedInvocationPtr enqueueDelayed(const Executable& func, D delay) {
        return enqueueInvocation(DelayedInvocation::Delayed(
            func, std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class D>
    DelayedInvocationPtr enqueueDelayed(Executable&& func, D delay) {
        return enqueueInvocation(DelayedInvocation::Delayed(
            std::move(func),
            std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class D>
    DelayedInvocationPtr enqueueInterval(const Executable& func, D delay) {
        return enqueueInvocation(DelayedInvocation::Interval(
            func, std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

    template<class D>
    DelayedInvocationPtr enqueueInterval(Executable&& func, D delay) {
        return enqueueInvocation(DelayedInvocation::Interval(
            std::move(func),
            std::chrono::duration_cast<DelayedInvocation::Delay>(delay)));
    }

 private:
    DelayedInvocationPtr enqueueInvocation(DelayedInvocationPtr invocation) {
        std::lock_guard guard(delayedQueueLock);
        delayedQueue.push(invocation);
        return invocation;
    }

    void executePending() {
        bool executed;
        do {
            executed = false;
            auto delayedInvocation = popDelayedQueue();
            if (delayedInvocation != nullptr) {
                delayedInvocation->executeIfNotCancelled();
                if (!delayedInvocation->isCancelled()
                        && delayedInvocation->isInterval()) {
                    delayedInvocation->setTimePoint();
                    enqueueInvocation(delayedInvocation);
                }
                executed = true;
            } else {
                auto immediateInvocation = popImmediateQueue();
                if (immediateInvocation != nullptr) {
                    immediateInvocation();
                    executed = true;
                }
            }
        } while (executed);
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

    Executable popImmediateQueue() {
        std::lock_guard guard(immediateQueueLock);
        if (immediateQueue.empty()) {
            return nullptr;
        }
        auto top = immediateQueue.front();
        immediateQueue.pop();
        return top;
    }

    std::atomic<std::chrono::nanoseconds> tick = std::chrono::nanoseconds(20);
    std::atomic_bool running = false;

    mutable std::mutex immediateQueueLock;
    std::queue<Executable> immediateQueue;

    mutable std::mutex delayedQueueLock;
    std::priority_queue<DelayedInvocationPtr,
                        std::vector<DelayedInvocationPtr>,
                        DelayedInvocation::Compare> delayedQueue;

};

}
