#pragma once

#include <mcga/threading/base/thread_pool_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class EventLoopThreadPoolConstruct : public base::ThreadPoolWrapper<W> {
 public:
    using Object = typename W::Object;
    using DelayedInvocation = typename W::DelayedInvocation;
    using DelayedInvocationPtr = typename W::DelayedInvocationPtr;

    using base::ThreadPoolWrapper<W>::ThreadPoolWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopThreadPoolConstruct);

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

}  // namespace mcga::threading::constructs
