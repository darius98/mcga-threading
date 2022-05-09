#pragma once

#include <atomic>
#include <thread>

namespace mcga::threading::base {

template<class W>
class ThreadWrapperBase {
  public:
    std::size_t sizeApprox() const {
        return worker.sizeApprox();
    }

  protected:
    using Processor = typename W::Processor;

    W* getWorker() {
        return &worker;
    }

    void acquireStartOrStop() {
        while (isInStartOrStop.test_and_set()) {
            std::this_thread::yield();
        }
    }

    void releaseStartOrStop() {
        isInStartOrStop.clear();
    }

    void startThread(std::atomic_bool* started, Processor* processor) {
        workerThread = std::thread([this, started, processor]() {
            started->store(true);
            this->worker.start(started, processor);
        });
        while (!started->load()) {
            std::this_thread::yield();
        }
    }

    void startThreadEmbedded(std::atomic_bool* started, Processor* processor) {
        std::atomic_bool localStarted = false;
        this->workerThread
          = std::thread([this, &localStarted, started, processor]() {
                localStarted = true;
                this->worker.start(started, processor);
            });
        while (!localStarted) {
            std::this_thread::yield();
        }
    }

    void tryJoin() {
        if (workerThread.joinable()) {
            // Since no other thread can enter start() or stop() while we are
            // here, nothing can happen that turns joinable() into
            // not-joinable() at this point (between the check and the join()).
            workerThread.join();
        }
    }

    W worker;
    std::thread workerThread;
    std::atomic_flag isInStartOrStop = ATOMIC_FLAG_INIT;
};

template<class W>
class ThreadWrapper : public ThreadWrapperBase<W> {
  public:
    using Wrapped = W;
    using Processor = typename W::Processor;
    using Task = typename W::Task;

    template<class... Args>
    explicit ThreadWrapper(Args&&... args)
            : started(false), processor(std::forward<Args>(args)...) {
    }

    ThreadWrapper(const ThreadWrapper&) = delete;
    ThreadWrapper(ThreadWrapper&&) = delete;

    ThreadWrapper& operator=(const ThreadWrapper&) = delete;
    ThreadWrapper& operator=(ThreadWrapper&&) = delete;

    ~ThreadWrapper() {
        stopRaw();
    }

    bool isRunning() const {
        return started.load();
    }

    void start() {
        this->acquireStartOrStop();
        if (!isRunning()) {
            this->startThread(&started, &processor);
        }
        this->releaseStartOrStop();
    }

    void stop() {
        stopRaw();
        this->releaseStartOrStop();
    }

    Processor* getProcessor() {
        return &processor;
    }

  private:
    void stopRaw() {
        this->acquireStartOrStop();
        started = false;
        this->tryJoin();
    }

    std::atomic_bool started;
    Processor processor;
};

template<class W>
class EmbeddedThreadWrapper : public ThreadWrapperBase<W> {
  public:
    using Wrapped = W;
    using Processor = typename W::Processor;
    using Task = typename W::Task;

    explicit EmbeddedThreadWrapper(std::atomic_bool* started,
                                   Processor* processor)
            : started(started), processor(processor) {
    }

    EmbeddedThreadWrapper(const EmbeddedThreadWrapper&) = delete;
    EmbeddedThreadWrapper(EmbeddedThreadWrapper&&) = delete;

    EmbeddedThreadWrapper& operator=(const EmbeddedThreadWrapper&) = delete;
    EmbeddedThreadWrapper& operator=(EmbeddedThreadWrapper&&) = delete;

    ~EmbeddedThreadWrapper() {
        stopRaw();
    }

    bool isRunning() const {
        return started->load();
    }

    void start() {
        this->acquireStartOrStop();
        this->startThreadEmbedded(started, processor);
        this->releaseStartOrStop();
    }

    void stop() {
        stopRaw();
        this->releaseStartOrStop();
    }

    Processor* getProcessor() {
        return processor;
    }

  private:
    void stopRaw() {
        this->acquireStartOrStop();
        this->tryJoin();
    }

    std::atomic_bool* started;
    Processor* processor;

    template<class T, class I>
    friend class ThreadPoolWrapper;
};

}  // namespace mcga::threading::base
