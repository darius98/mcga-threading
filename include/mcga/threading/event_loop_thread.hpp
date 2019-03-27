#pragma once

#include <mutex>
#include <thread>

#include "event_loop.hpp"

namespace mcga::threading {

class EventLoopThread {
 public:
    EventLoopThread() = default;

    template<class _Rep, class _Ratio>
    explicit EventLoopThread(
            const std::chrono::duration<_Rep, _Ratio>& tick): loop(tick) {}

    ~EventLoopThread();

    template<class _Rep, class _Ratio>
    void setTick(const std::chrono::duration<_Rep, _Ratio>& tick) {
        loop.setTick(tick);
    }

    std::size_t size() const;

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

}
