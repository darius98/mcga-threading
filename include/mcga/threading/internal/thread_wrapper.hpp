#pragma once

#include <atomic>
#include <thread>

#include "disallow_copy_and_move.hpp"

namespace mcga::threading::internal {

template<class W>
class ThreadWrapper {
 public:
    ThreadWrapper() = default;

    DISALLOW_COPY_AND_MOVE(ThreadWrapper);

    ~ThreadWrapper() {
        // This time, spin until we can actually stop this, and then join the
        // worker thread.
        //
        // This is to ensure there is no "thread leak" after destroying the
        // object.
        while (isInStartOrStop.test_and_set()) {
        }
        stopRaw();
    }

    std::size_t sizeApprox() const {
        return worker.sizeApprox();
    }

    bool isRunning() const {
        return started;
    }

    void start() {
        if (isInStartOrStop.test_and_set()) {
            // Ensure that only one thread's call to start()/stop() actually
            // does anything. This flag is cleared at the end of the method.
            return;
        }
        if (!started) {
            try {
                workerThread = std::thread([this]() {
                    started.store(true);
                    worker.start(&started);
                });
            } catch(const std::system_error& err) {
                isInStartOrStop.clear();
                throw err;
            }
            while (!started) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(1));
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
        started.store(false);
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
    std::atomic_bool started = false;
    std::thread workerThread;
};

}  // namespace mcga::threading::internal