#pragma once

#include <atomic>
#include <thread>

#include <mcga/threading/base/disallow_copy_and_move.hpp>

namespace mcga::threading::base {

template<class W>
class ThreadWrapper {
 public:
    ThreadWrapper() {
        started = new std::atomic_bool(false);
    }

    explicit ThreadWrapper(volatile std::atomic_bool* started):
            ownStartedFlag(false), started(started) {}

    DISALLOW_COPY_AND_MOVE(ThreadWrapper);

    ~ThreadWrapper() {
        // This time, spin until we can actually stop this, and then join the
        // worker thread.
        while (isInStartOrStop.test_and_set()) {
        }
        stopRaw();
        if (ownStartedFlag) {
            delete started;
        }
    }

    std::size_t sizeApprox() const {
        return worker.sizeApprox();
    }

    bool isRunning() const {
        return started->load();
    }

    void start() {
        if (isInStartOrStop.test_and_set()) {
            // Ensure that only one thread's call to start()/stop() actually
            // does anything. This flag is cleared at the end of the method.
            return;
        }
        if (ownStartedFlag) {
            if (!started->load()) {
                workerThread = std::thread([this]() {
                    started->store(true);
                    worker.start(started);
                });
                while (!started->load()) {
                    std::this_thread::yield();
                }
            }
        } else {
            std::atomic_bool localStarted = false;
            workerThread = std::thread([this, &localStarted]() {
                localStarted.store(true);
                worker.start(started);
            });
            while (!localStarted.load()) {
                std::this_thread::yield();
            }
        }
        isInStartOrStop.clear();
    }

    void stop() {
        if (isInStartOrStop.test_and_set()) {
            // Ensure that only one thread's call to start()/stop() actually
            // does anything. This flag is cleared at the end of the method.
            return;
        }
        stopRaw();
        isInStartOrStop.clear();
    }

 private:
    void stopRaw() {
        if (ownStartedFlag) {
            started->store(false);
        }
        if (workerThread.joinable()) {
            // Since no other thread can enter start() or stop() while we are
            // here, nothing can happen that turns joinable() into
            // not-joinable() at this point (between the check and the join()).
            workerThread.join();
        }
    }

 protected:
    W worker;

 private:
    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
    bool ownStartedFlag = true;
    volatile std::atomic_bool* started;
    std::thread workerThread;
};

}  // namespace mcga::threading::base
