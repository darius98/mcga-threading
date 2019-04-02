#pragma once

#include <mutex>
#include <thread>
#include <vector>

#include <concurrentqueue.h>

#include "internal/thread_wrapper.hpp"

namespace mcga::threading {

class Worker {
 public:
    using Executable = std::function<void()>;

    Worker();
    ~Worker() = default;

    DISALLOW_COPY_AND_MOVE(Worker);

    size_t sizeApprox() const;

    bool isRunning() const;
    void start();
    void stop();

    void enqueue(const Executable& job);
    void enqueue(Executable&& job);

 private:
    void run();

    std::atomic_bool running = false;

    moodycamel::ConcurrentQueue<Executable> queue;
    moodycamel::ConsumerToken queueConsumerToken;
    std::vector<Executable> queueBuffer;
    std::atomic_size_t numBuffered = 0;
};

class WorkerThread : public internal::ThreadWrapper<Worker> {
 public:
    using Executable = Worker::Executable;

    WorkerThread() = default;
    ~WorkerThread() = default;

    DISALLOW_COPY_AND_MOVE(WorkerThread);

    void enqueue(const Executable& func);
    void enqueue(Executable&& func);
};

}  // namespace mcga::threading
