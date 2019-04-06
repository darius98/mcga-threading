#pragma once

#include <vector>

#include <concurrentqueue.h>

#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/loop_tick_duration.hpp>
#include <mcga/threading/base/immediate_queue_wrapper.hpp>

namespace mcga::threading::base {

template<class Processor>
class Worker:
    private Processor,
    public base::ImmediateQueueWrapper<Processor, typename Processor::Task> {
 public:
    using Task = typename Processor::Task;

    using Processor::Processor;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(Worker);

    ~Worker() = default;

    size_t sizeApprox() const {
        return this->getImmediateQueueSize();
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            this->executeImmediate(this);
            std::this_thread::sleep_for(base::loopTickDuration);
        }
    }
};

}  // namespace mcga::threading::base
