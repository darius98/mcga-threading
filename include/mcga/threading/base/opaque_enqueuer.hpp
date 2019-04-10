#pragma once

#include <functional>

#include <mcga/threading/base/delayed_task.hpp>

namespace mcga::threading::base {

template<class Task>
class OpaqueWorkerEnqueuer {
 public:
    void enqueue(Task task) {
        enqueuer(std::move(task));
    }

 protected:
    explicit OpaqueWorkerEnqueuer(std::function<void(Task task)> enqueuer):
            enqueuer(std::move(enqueuer)) {}

 private:
    std::function<void(Task task)> enqueuer;

template<class Processor, class I> friend class Worker;
};

template<class Task>
class OpaqueEventLoopEnqueuer: public OpaqueWorkerEnqueuer<Task> {
 public:
    using DelayedTaskPtr = std::shared_ptr<DelayedTask<Task>>;

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueDelayed(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return delayedEnqueuer(
                std::move(task), std::chrono::duration_cast<Delay>(delay));
    }

    template<class Rep, class Ratio>
    DelayedTaskPtr enqueueInterval(
            Task task, const std::chrono::duration<Rep, Ratio>& delay) {
        return intervalEnqueuer(
                std::move(task), std::chrono::duration_cast<Delay>(delay));
    }

 private:
    using Delay = typename DelayedTask<Task>::Delay;

    OpaqueEventLoopEnqueuer(
        std::function<void(Task task)> enqueuer,
        std::function<DelayedTaskPtr(Task task, const Delay&)> delayedEnqueuer,
        std::function<DelayedTaskPtr(Task task, const Delay&)> intervalEnqueuer)
        : OpaqueWorkerEnqueuer<Task>(std::move(enqueuer)),
          delayedEnqueuer(std::move(delayedEnqueuer)),
          intervalEnqueuer(std::move(intervalEnqueuer)) {}

    std::function<DelayedTaskPtr(Task task, const Delay&)> delayedEnqueuer;
    std::function<DelayedTaskPtr(Task task, const Delay&)> intervalEnqueuer;

template<class Processor, class I, class D> friend class EventLoop;
};

}  // namespace mcga::threading::base

namespace mcga::threading {

using base::OpaqueWorkerEnqueuer;
using base::OpaqueEventLoopEnqueuer;

} // namespace mcga::threading
