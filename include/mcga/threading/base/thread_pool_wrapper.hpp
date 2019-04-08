#pragma once

#include <atomic>
#include <thread>

#include <mcga/threading/base/disallow_copy_and_move.hpp>
#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::base {

template<class Thread, class Idx = std::atomic_size_t>
class ThreadPoolWrapper {
 public:
    struct NumThreads {
        std::size_t numThreads;

        explicit NumThreads(std::size_t numThreads): numThreads(numThreads) {}
    };

    using Processor = typename Thread::Processor;

    template<class... Args>
    explicit ThreadPoolWrapper(NumThreads numThreads, Args&&... args):
            processor(std::forward<Args>(args)...) {
        threads.reserve(numThreads.numThreads);
        for (int i = 0; i < numThreads.numThreads; i += 1) {
            threads.push_back(
                new Thread(Thread::insideThreadPool, &started, &processor));
        }
    }

    template<class... Args>
    explicit ThreadPoolWrapper(Args&&... args):
            processor(std::forward<Args>(args)...) {
        threads.reserve(std::thread::hardware_concurrency());
        for (int i = 0; i < std::thread::hardware_concurrency(); i += 1) {
            threads.push_back(
                new Thread(Thread::insideThreadPool, &started, &processor));
        }
    }

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(ThreadPoolWrapper);

    ~ThreadPoolWrapper() {
        stopRaw();
        for (Thread* thread: threads) {
            delete thread;
        }
    }

    std::size_t sizeApprox() const {
        std::size_t size = 0;
        for (const Thread* thread: threads) {
            size += thread->sizeApprox();
        }
        return size;
    }

    bool isRunning() const {
        return started.load();
    }

    void start() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
        if (!started.load()) {
            started.store(true);
            for (Thread* thread: threads) {
                thread->start();
            }
        }
        isInStartOrStop.clear();
    }

    void stop() {
        stopRaw();
        isInStartOrStop.clear();
    }

    Processor* getProcessor() {
        return processor;
    }

 protected:
    Thread* nextThread() {
        return threads[(++currentThreadId) % threads.size()];
    }

 private:
    void stopRaw() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
        if (started.load()) {
            started.store(false);
            for (Thread* thread: threads) {
                thread->stop();
            }
        }
    }

    Processor processor;
    Idx currentThreadId = 0;
    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
    volatile std::atomic_bool started = false;
    std::vector<Thread*> threads;
};

}  // namespace mcga::threading::base
