#pragma once

#include <concurrentqueue.h>

#include "method_checks.hpp"

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
        return queue.size_approx() + bufferSize;
    }

    template<class Enqueuer>
    bool executeImmediate(Processor* processor, Enqueuer* enqueuer) {
        auto queueSize = queue.size_approx();
        if (queueSize == 0) {
            return false;
        }
        if (queueSize > buffer.size()) {
            buffer.resize(queueSize);
        }
        bufferSize
          = queue.try_dequeue_bulk(queueToken, buffer.begin(), buffer.size());
        for (size_t i = 0; bufferSize > 0; --bufferSize, ++i) {
            if constexpr (hasExecuteTaskWithEnqueuer<Processor, Enqueuer>) {
                processor->executeTask(buffer[i], enqueuer);
            } else {
                processor->executeTask(buffer[i]);
            }
        }
        return true;
    }

  private:
    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ConsumerToken queueToken{queue};
    std::vector<Task> buffer;
    volatile std::size_t bufferSize = 0;
};

}  // namespace mcga::threading::base
