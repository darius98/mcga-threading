#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "thread_wrapper.hpp"

namespace mcga::threading::base {

template<class W, class Idx>
class ThreadPoolWrapper {
  private:
    using Thread = EmbeddedThreadWrapper<W>;

  public:
    struct NumThreads {
        std::size_t numThreads;

        explicit NumThreads(std::size_t numThreads): numThreads(numThreads) {
        }
    };

    using Wrapped = W;
    using Processor = typename W::Processor;
    using Task = typename W::Task;

    template<class... Args>
    explicit ThreadPoolWrapper(NumThreads numThreads, Args&&... args)
            : processor(std::forward<Args>(args)...) {
        threads.reserve(numThreads.numThreads);
        for (int i = 0; i < numThreads.numThreads; i += 1) {
            threads.push_back(std::make_unique<Thread>(&started, &processor));
        }
    }

    template<class... Args>
    explicit ThreadPoolWrapper(Args&&... args)
            : ThreadPoolWrapper(NumThreads(std::thread::hardware_concurrency()),
                                std::forward<Args>(args)...) {
    }

    ThreadPoolWrapper(const ThreadPoolWrapper&) = delete;
    ThreadPoolWrapper(ThreadPoolWrapper&&) = delete;

    ThreadPoolWrapper& operator=(const ThreadPoolWrapper&) = delete;
    ThreadPoolWrapper& operator=(ThreadPoolWrapper&&) = delete;

    ~ThreadPoolWrapper() {
        stopRaw();
    }

    std::size_t sizeApprox() const {
        std::size_t size = 0;
        for (const std::unique_ptr<Thread>& thread: threads) {
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
            for (std::unique_ptr<Thread>& thread: threads) {
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
    Wrapped* getWorker() {
        return threads[(++currentThreadId) % threads.size()]->getWorker();
    }

  private:
    void stopRaw() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
        if (started.load()) {
            started.store(false);
            for (std::unique_ptr<Thread>& thread: threads) {
                thread->stop();
            }
        }
    }

    Processor processor;
    Idx currentThreadId = 0;
    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
    std::atomic_bool started = false;
    std::vector<std::unique_ptr<Thread>> threads;
};

}  // namespace mcga::threading::base
