#pragma once

#include <mutex>
#include <thread>

#include "event_loop.hpp"

namespace mcga::threading {

class EventLoopThread {
 public:
    EventLoopThread() = default;

    EventLoopThread(EventLoopThread&&) = delete;
    EventLoopThread(const EventLoopThread&) = delete;

    EventLoopThread& operator=(EventLoopThread&&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;

    ~EventLoopThread();

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
        return loop.enqueueDelayed(func, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueDelayed(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return loop.enqueueDelayed(std::move(func), delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            const Executable& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return loop.enqueueInterval(func, delay);
    }

    template<class _Rep, class _Ratio>
    DelayedInvocationPtr enqueueInterval(
            Executable&& func,
            const std::chrono::duration<_Rep, _Ratio>& delay) {
        return loop.enqueueInterval(std::move(func), delay);
    }

 private:
    mutable std::mutex startStopMutex;
    std::atomic_bool isStarted = false;
    EventLoop loop;
    std::thread loopThread;
};

}  // namespace mcga::threading
