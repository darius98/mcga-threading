#pragma once

#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class EventLoopThreadConstruct : public base::ThreadWrapper<W> {
 public:
    using Task = typename W::Task;
    using Delay = typename W::Delay;
    using DelayedTaskPtr = typename W::DelayedTaskPtr;

    using base::ThreadWrapper<W>::ThreadWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoopThreadConstruct);

    ~EventLoopThreadConstruct() = default;

    void enqueue(Task task) {
        this->getWorker()->enqueue(std::move(task));
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueDelayed(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->getWorker()->enqueueDelayed(
                std::move(task), std::chrono::duration_cast<Delay>(delay));
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueInterval(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return this->getWorker()->enqueueInterval(
                std::move(task), std::chrono::duration_cast<Delay>(delay));
    }
};

}  // namespace mcga::threading::constructs
