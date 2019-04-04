#pragma once

#include <concurrentqueue.h>

#include "disallow_copy_and_move.hpp"
#include "loop_tick_duration.hpp"
#include "thread_pool_wrapper.hpp"
#include "thread_wrapper.hpp"

namespace mcga::threading::internal {

template<class Exec>
class WorkerConstruct : private Exec {
 public:
    using Object = typename Exec::Object;

    WorkerConstruct(): Exec(), queueConsumerToken(queue) {}

    DISALLOW_COPY_AND_MOVE(WorkerConstruct);

    ~WorkerConstruct() = default;

    size_t sizeApprox() const {
        return queue.size_approx() + numBuffered;
    }

    void start(volatile std::atomic_bool* running) {
        while (running->load()) {
            run();
            std::this_thread::sleep_for(loopTickDuration);
        }
    }

    void enqueue(const Object& obj) {
        queue.enqueue(obj);
    }

    void enqueue(Object&& obj) {
        queue.enqueue(std::move(obj));
    }

 private:
    void run() {
        auto queueSize = queue.size_approx();
        if (queueSize == 0) {
            return;
        }
        if (queueSize > queueBuffer.size()) {
            queueBuffer.resize(queueSize);
        }
        numBuffered = queue.try_dequeue_bulk(
            queueConsumerToken,
            queueBuffer.begin(),
            queueBuffer.size());
        for (size_t i = 0; numBuffered > 0; --numBuffered, ++ i) {
            this->handleObject(queueBuffer[i]);
        }
    }

    moodycamel::ConcurrentQueue<Object> queue;
    moodycamel::ConsumerToken queueConsumerToken;
    std::vector<Object> queueBuffer;
    std::atomic_size_t numBuffered = 0;
};

template<class W>
class WorkerThreadConstruct : public ThreadWrapper<W> {
 public:
    using Object = typename W::Object;

    WorkerThreadConstruct() = default;
    ~WorkerThreadConstruct() = default;

    DISALLOW_COPY_AND_MOVE(WorkerThreadConstruct);

    void enqueue(const Object& func) {
        this->worker.enqueue(func);
    }

    void enqueue(Object&& func) {
        this->worker.enqueue(std::move(func));
    }

 private:
    explicit WorkerThreadConstruct(volatile std::atomic_bool* running):
            internal::ThreadWrapper<W>(running) {}

 friend class ThreadPoolWrapper<WorkerThreadConstruct>;
};

template<class W>
class WorkerThreadPoolConstruct : public ThreadPoolWrapper<W> {
 public:
    using Object = typename W::Object;

    using ThreadPoolWrapper<W>::ThreadPoolWrapper;

    DISALLOW_COPY_AND_MOVE(WorkerThreadPoolConstruct);

    ~WorkerThreadPoolConstruct() = default;

    void enqueue(const Object& func) {
        this->nextThread()->enqueue(func);
    }

    void enqueue(Object&& func) {
        this->nextThread()->enqueue(std::move(func));
    }
};

}  // namespace mcga::threading::internal