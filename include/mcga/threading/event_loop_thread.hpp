#pragma once

#include <mutex>
#include <thread>

#include "event_loop.hpp"
#include "internal/thread_wrapper.hpp"

namespace mcga::threading {

class EventLoopThread: public internal::ThreadWrapper<EventLoop> {
 public:
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
