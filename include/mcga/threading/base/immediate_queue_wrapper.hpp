#pragma once

#include <concurrentqueue.h>

#include <memory>

namespace mcga::threading::base {

template<class Processor>
class ImmediateQueueWrapper {
  private:
    static constexpr std::size_t kInitialBufferCapacity = 16;

  public:
    using Task = typename Processor::Task;

    void enqueue(Task task) {
        queue.enqueue(std::move(task));
    }

  protected:
    std::size_t getImmediateQueueSize() const {
        return queue.size_approx() + bufferSize;
    }

    bool executeImmediate(Processor* processor) {
        auto queueSize = queue.size_approx();
        if (queueSize == 0) {
            return false;
        }
        if (queueSize > bufferCapacity) {
            bufferCapacity *= 2;
            buffer = std::make_unique<Task[]>(bufferCapacity);
        }
        bufferSize
          = queue.try_dequeue_bulk(queueToken, buffer.get(), bufferCapacity);
        for (size_t i = 0; bufferSize > 0; --bufferSize, ++i) {
            processor->executeTask(buffer[i]);
        }
        return true;
    }

  private:
    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ConsumerToken queueToken{queue};
    std::size_t bufferCapacity = kInitialBufferCapacity;
    std::unique_ptr<Task[]> buffer = std::make_unique<Task[]>(bufferCapacity);
    std::atomic_size_t bufferSize = 0;
};

}  // namespace mcga::threading::base
