#pragma once

namespace mcga::threading::base {

template<class Processor, class Task>
class ImmediateQueueWrapper {
 public:
    void enqueue(const Task& task) {
        queue.enqueue(task);
    }

    void enqueue(Task&& task) {
        queue.enqueue(std::move(task));
    }

 protected:
    std::size_t getImmediateQueueSize() const {
        return queue.size_approx() + queueBufferSize;
    }

    void executeImmediate(Processor* processor) {
        auto queueSize = queue.size_approx();
        if (queueSize > 0) {
            if (queueSize > queueBuffer.size()) {
                queueBuffer.resize(queueSize);
            }
            queueBufferSize = queue.try_dequeue_bulk(
                    queueToken,
                    queueBuffer.begin(),
                    queueBuffer.size());
            for (size_t i = 0; queueBufferSize > 0; --queueBufferSize, ++ i) {
                processor->executeTask(std::move(queueBuffer[i]));
            }
        }
    }

 private:
    moodycamel::ConcurrentQueue<Task> queue;
    moodycamel::ConsumerToken queueToken{queue};
    std::vector<Task> queueBuffer;
    std::size_t queueBufferSize = 0;
};

} // namespace mcga::threading::base
