#pragma once

#include <queue>
#include <thread>
#include <vector>

#include <mcga/threading/base/delayed_queue_wrapper.hpp>
#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/immediate_queue_wrapper.hpp>
#include <mcga/threading/base/single_producer_immediate_queue_wrapper.hpp>
#include <mcga/threading/base/loop_tick_duration.hpp>

namespace mcga::threading::base {

template<class P,
         class ImmediateQueue = base::ImmediateQueueWrapper<P>,
         class DelayedQueue = base::DelayedQueueWrapper<P>>
class EventLoop: public DelayedQueue, public ImmediateQueue {
 public:
    using Processor = P;
    using Task = typename Processor::Task;

    EventLoop() = default;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoop);

    ~EventLoop() = default;

    std::size_t sizeApprox() const {
        return this->getImmediateQueueSize() + this->getDelayedQueueSize();
    }

    void start(volatile std::atomic_bool* running, Processor* processor) {
        while (running->load()) {
            while (this->executeDelayed(processor)
                    || this->executeImmediate(processor)) {
                std::this_thread::yield();
            }
            std::this_thread::sleep_for(base::loopTickDuration);
        }
    }
};

template<class P>
using SingleProducerEventLoop
        = EventLoop<P, SingleProducerImmediateQueueWrapper<P>>;

}  // namespace mcga::threading::base
