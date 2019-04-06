#pragma once

#include <chrono>

#include <concurrentqueue.h>

#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class EventLoopThreadConstruct : public base::ThreadWrapper<W> {
 public:
    using Task = typename W::Task;
    using DelayedTask = typename W::DelayedTask;
    using DelayedTaskPtr = typename W::DelayedTaskPtr;

    using base::ThreadWrapper<W>::ThreadWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopThreadConstruct);

    ~EventLoopThreadConstruct() = default;

    void enqueue(const Task& task) {
        this->worker.enqueue(task);
    }

    void enqueue(Task&& task) {
        this->worker.enqueue(std::move(task));
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueDelayed(
            const Task& task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->worker.enqueueDelayed(task, delay);
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueDelayed(
            Task&& task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->worker.enqueueDelayed(std::move(task), delay);
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueInterval(
            const Task& task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->worker.enqueueInterval(task, delay);
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueInterval(
            Task&& task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->worker.enqueueInterval(std::move(task), delay);
    }
};

}  // namespace mcga::threading::constructs
