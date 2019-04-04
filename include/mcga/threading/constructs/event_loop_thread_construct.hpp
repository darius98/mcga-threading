#pragma once

#include <chrono>

#include <concurrentqueue.h>

#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class EventLoopThreadConstruct : public base::ThreadWrapper<W> {
 public:
    using Object = typename W::Object;
    using DelayedInvocation = typename W::DelayedInvocation;
    using DelayedInvocationPtr = typename W::DelayedInvocationPtr;

    using base::ThreadWrapper<W>::ThreadWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopThreadConstruct);

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
};

}  // namespace mcga::threading::constructs
