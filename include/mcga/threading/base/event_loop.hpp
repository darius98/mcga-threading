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

template<class Processor,
         class ImmediateQueue = base::ImmediateQueueWrapper<Processor>,
         class DelayedQueue = base::DelayedQueueWrapper<Processor>>
class EventLoop: private Processor, public DelayedQueue, public ImmediateQueue {
 public:
    // TODO: This should not be public
    using ThreadIndex = std::atomic_size_t;

    using Task = typename Processor::Task;

    using Processor::Processor;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(EventLoop);

    ~EventLoop() = default;

    std::size_t sizeApprox() const {
        return this->getImmediateQueueSize() + this->getDelayedQueueSize();
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            while (this->executeDelayed(this) || this->executeImmediate(this)) {
                std::this_thread::yield();
            }
            std::this_thread::sleep_for(base::loopTickDuration);
        }
    }
};

template<class Processor>
class SingleProducerEventLoop: public EventLoop
        <Processor, SingleProducerImmediateQueueWrapper<Processor>> {
 public:
    // TODO: This should not be public
    using ThreadIndex = std::size_t;

    using EventLoop<Processor, SingleProducerImmediateQueueWrapper<Processor>>
            ::EventLoop;
};

}  // namespace mcga::threading::base
