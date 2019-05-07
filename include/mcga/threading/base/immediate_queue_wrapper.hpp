#pragma once

#include <concurrentqueue.h>

#include <mcga/threading/base/method_checks.hpp>

namespace mcga::threading::base {

template<class Processor>
class ImmediateQueueWrapper {
  public:
    using Task = typename Processor::Task;

    void enqueue(Task task) {
        queue.enqueue(std::move(task));
    }

  protected:
    std::size_t getImmediateQueueSize() const {
        return queue.size_approx() + queueBufferSize;
    }

    template<class Enqueuer>
    bool executeImmediate(Processor* processor, Enqueuer* enqueuer) {
        auto queueSize = queue.size_approx();
        if (queueSize == 0) {
            return false;
        }
        if (queueSize > queueBuffer.size()) {
            queueBuffer.resize(queueSize);
        }
        queueBufferSize = queue.try_dequeue_bulk(
          queueToken, queueBuffer.begin(), queueBuffer.size());
        for (size_t i = 0; queueBufferSize > 0; --queueBufferSize, ++i) {
            if constexpr (hasExecuteTaskWithEnqueuer<Processor,
                                                              Enqueuer>) {
                processor->executeTask(queueBuffer[i], enqueuer);
            } else {
                processor->executeTask(queueBuffer[i]);
            }
        }
        return true;
    }

  private:
    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ConsumerToken queueToken{queue};
    std::vector<Task> queueBuffer;
    volatile std::size_t queueBufferSize = 0;
};

}  // namespace mcga::threading::base
