#pragma once

#include <concurrentqueue.h>

#include <mcga/threading/base/execute_task_variants.hpp>

namespace mcga::threading::base {

template<class Processor>
class SingleProducerImmediateQueueWrapper {
  public:
    using Task = typename Processor::Task;

    void enqueue(Task task) {
        queue.enqueue(queueProducerToken, std::move(task));
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
          queueConsumerToken, queueBuffer.begin(), queueBuffer.size());
        for (size_t i = 0; queueBufferSize > 0; --queueBufferSize, ++i) {
            executeTask(std::move(queueBuffer[i]), processor, enqueuer);
        }
        return true;
    }

  private:
    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ProducerToken queueProducerToken{queue};
    moodycamel::ConsumerToken queueConsumerToken{queue};
    std::vector<Task> queueBuffer;
    std::size_t queueBufferSize = 0;
};

}  // namespace mcga::threading::base
