#pragma once

#include <vector>

#include <concurrentqueue.h>

#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/loop_tick_duration.hpp>

namespace mcga::threading::constructs {

template<class Processor>
class WorkerConstruct : private Processor {
 public:
    using Task = typename Processor::Task;

    using Processor::Processor;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(WorkerConstruct);

    ~WorkerConstruct() = default;

    size_t sizeApprox() const {
        return queue.size_approx() + numBuffered;
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            run();
            std::this_thread::sleep_for(base::loopTickDuration);
        }
    }

    void enqueue(const Task& task) {
        queue.enqueue(task);
    }

    void enqueue(Task&& obj) {
        queue.enqueue(std::move(obj));
    }

 private:
    void run() {
        auto queueSize = queue.size_approx();
        if (queueSize == 0) {
            return;
        }
        if (queueSize > queueBuffer.size()) {
            queueBuffer.resize(queueSize);
        }
        numBuffered = queue.try_dequeue_bulk(
            queueConsumerToken,
            queueBuffer.begin(),
            queueBuffer.size());
        for (size_t i = 0; numBuffered > 0; --numBuffered, ++ i) {
            this->executeTask(queueBuffer[i]);
        }
    }

    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ConsumerToken queueConsumerToken{queue};
    std::vector<Task> queueBuffer;
    std::size_t numBuffered = 0;
};

}  // namespace mcga::threading::constructs