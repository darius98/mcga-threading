#pragma once

#include <atomic>
#include <thread>

#include <mcga/threading/base/disallow_copy_and_move.hpp>

namespace mcga::threading::base {

template<class W>
class ThreadWrapper {
 public:
    template<class... Args>
    explicit ThreadWrapper(volatile std::atomic_bool* started, Args&&... args):
            worker(std::forward<Args>(args)...),
            ownStartedFlag(false),
            started(started) {}

    template<class... Args>
    explicit ThreadWrapper(Args&&... args):
            worker(std::forward<Args>(args)...) {
        started = new std::atomic_bool(false);
    }

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(ThreadWrapper);

    ~ThreadWrapper() {
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
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
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
        stopRaw();
        isInStartOrStop.clear();
    }

 private:
    void stopRaw() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
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
