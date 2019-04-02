#include "mcga/threading/worker_thread.hpp"

#include "mcga/threading/internal/loop_tick_duration.hpp"

using std::move;

namespace mcga::threading {

Worker::Worker(): queueConsumerToken(queue) {}

bool Worker::isRunning() const {
    return running;
}

size_t Worker::sizeApprox() const {
    return queue.size_approx() + numBuffered;
}

void Worker::start() {
    if (!running.exchange(true)) {
        while (running.load()) {
            run();
            std::this_thread::sleep_for(internal::loopTickDuration);
        }
    }
}

void Worker::stop() {
    running.store(false);
}

void Worker::enqueue(const Executable& job) {
    queue.enqueue(job);
}

void Worker::enqueue(Executable&& job) {
    queue.enqueue(move(job));
}

void Worker::run() {
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
        queueBuffer[i]();
    }
}

void WorkerThread::enqueue(const Executable& func) {
    worker.enqueue(func);
}

void WorkerThread::enqueue(Executable&& func) {
    worker.enqueue(move(func));
}

}  // namespace mcga::threading
