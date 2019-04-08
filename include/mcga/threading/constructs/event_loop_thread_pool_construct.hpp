#pragma once

#include <atomic>

#include <mcga/threading/base/thread_pool_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W, class Idx = std::atomic_size_t>
class EventLoopThreadPoolConstruct: public base::ThreadPoolWrapper<W, Idx> {
 public:
    using Task = typename W::Task;
    using DelayedTaskPtr = typename W::DelayedTaskPtr;

    using base::ThreadPoolWrapper<W, Idx>::ThreadPoolWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopThreadPoolConstruct);

    ~EventLoopThreadPoolConstruct() = default;

    void enqueue(Task task) {
        this->nextThread()->enqueue(std::move(task));
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueDelayed(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->nextThread()->enqueueDelayed(std::move(task), delay);
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueInterval(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->nextThread()->enqueueInterval(std::move(task), delay);
    }
};

}  // namespace mcga::threading::constructs
