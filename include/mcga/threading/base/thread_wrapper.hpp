#pragma once

#include <atomic>
#include <thread>

#include <mcga/threading/base/disallow_copy_and_move.hpp>

namespace mcga::threading::base {

template<class W>
class ThreadWrapper {
  private:
    struct InsideThreadPoolT {};

    static constexpr InsideThreadPoolT insideThreadPool;

  public:
    using Wrapped = W;
    using Processor = typename W::Processor;
    using Task = typename W::Task;

    explicit ThreadWrapper(InsideThreadPoolT /*unused*/,
                           volatile std::atomic_bool* started,
                           Processor* processor)
            : insidePool(true), processor(processor), started(started) {
    }

    template<class... Args>
    explicit ThreadWrapper(Args&&... args)
            : insidePool(false),
              processor(new Processor(std::forward<Args>(args)...)),
              started(new std::atomic_bool(false)) {
    }

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(ThreadWrapper);

    ~ThreadWrapper() {
        stopRaw();
        if (!insidePool) {
            delete started;
            delete processor;
        }
    }

    std::size_t sizeApprox() const {
        return worker.sizeApprox();
    }

    bool isRunning() const {
        return started->load();
    }

    void start() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
        if (!insidePool) {
            if (!started->load()) {
                workerThread = std::thread([this]() {
                    started->store(true);
                    worker.start(started, processor);
                });
                while (!started->load()) {
                    std::this_thread::yield();
                }
            }
        } else {
            volatile bool localStarted = false;
            workerThread = std::thread([this, &localStarted]() {
                localStarted = true;
                worker.start(started, processor);
            });
            while (!localStarted) {
                std::this_thread::yield();
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
    W* getWorker() {
        return &worker;
    }

  private:
    void stopRaw() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
        if (!insidePool) {
            started->store(false);
        }
        if (workerThread.joinable()) {
            // Since no other thread can enter start() or stop() while we are
            // here, nothing can happen that turns joinable() into
            // not-joinable() at this point (between the check and the join()).
            workerThread.join();
        }
    }

    bool insidePool;

    Wrapped worker;
    Processor* processor;

    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
    volatile std::atomic_bool* started;
    std::thread workerThread;

    template<class T, class I>
    friend class ThreadPoolWrapper;
};

}  // namespace mcga::threading::base
