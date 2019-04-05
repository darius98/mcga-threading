#pragma once

#include <atomic>
#include <thread>

#include <mcga/threading/base/disallow_copy_and_move.hpp>

namespace mcga::threading::base {

template<class Thread>
class ThreadPoolWrapper {
 public:
    template<class... Args>
    explicit ThreadPoolWrapper(
            Args&&... args,
            std::size_t numThreads = std::thread::hardware_concurrency()) {
        threads.reserve(numThreads);
        for (int i = 0; i < numThreads; i += 1) {
            threads.push_back(
                new Thread(&started, std::forward<Args>(args)...));
        }
    }

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(ThreadPoolWrapper);

    ~ThreadPoolWrapper() {
        stopRaw();
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
 protected:
    Thread* nextThread() {
        return threads[currentThreadId.fetch_add(1) % threads.size()];
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

    std::atomic_size_t currentThreadId = 0;
    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
    volatile std::atomic_bool started = false;
    std::vector<Thread*> threads;
};

}  // namespace mcga::threading::base
