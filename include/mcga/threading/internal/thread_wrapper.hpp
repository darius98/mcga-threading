#pragma once

#include <mutex>
#include <thread>

#include "disallow_copy_and_move.hpp"

namespace mcga::threading::internal {

template<class W>
class ThreadWrapper {
 public:
    ThreadWrapper() = default;

    DISALLOW_COPY_AND_MOVE(ThreadWrapper);

    ~ThreadWrapper() {
        stop();
    }

    std::size_t size() const {
        return worker.size();
    }

    bool isRunning() const {
        std::lock_guard guard(startStopMutex);
        return isStarted;
    }

    void start() {
        std::lock_guard guard(startStopMutex);
        if (isStarted) {
            return;
        }
        try {
            workerThread = std::thread([this]() {
                isStarted = true;
                worker.start();
            });
        } catch(const std::system_error& err) {
            guard.~lock_guard();
            throw err;
        }
        while (!isStarted) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        }
    }

    void stop() {
        std::lock_guard guard(startStopMutex);
        if (!isStarted) {
            return;
        }
        worker.stop();
        workerThread.join();
        isStarted = false;
    }

 protected:
    W worker;
 private:
    mutable std::mutex startStopMutex;
    std::atomic_bool isStarted = false;
    std::thread workerThread;
};

}  // namespace mcga::threading::internal