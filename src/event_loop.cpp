#include "mcga/threading/event_loop.hpp"

using std::lock_guard;
using std::move;
using std::size_t;
using std::this_thread::sleep_for;
using std::vector;
using namespace std::literals::chrono_literals;

namespace mcga::threading {

EventLoop::EventLoop(): immediateQueueToken(immediateQueue) {}

EventLoop::~EventLoop() {
    delete[] immediateQueueBuffer;
}

size_t EventLoop::size() const {
    return immediateQueue.size_approx() + getDelayedQueueSize();
}

bool EventLoop::isRunning() const {
    return running;
}

void EventLoop::start() {
    running = true;
    while (running) {
        executePending();
        sleep_for(20ns);
    }
}

void EventLoop::stop() {
    running = false;
}

void EventLoop::enqueue(const Executable& func) {
    immediateQueue.enqueue(func);
}

void EventLoop::enqueue(Executable&& func) {
    immediateQueue.enqueue(move(func));
}

DelayedInvocationPtr EventLoop::enqueue(DelayedInvocationPtr invocation) {
    lock_guard guard(delayedQueueLock);
    delayedQueue.push(invocation);
    return invocation;
}

void EventLoop::executePending() {
    bool executed;
    do {
        executed = false;
        auto delayedInvocation = popDelayedQueue();
        if (delayedInvocation != nullptr) {
            delayedInvocation->executeIfNotCancelled();
            if (!delayedInvocation->isCancelled()
                    && delayedInvocation->isInterval()) {
                delayedInvocation->setTimePoint();
                enqueue(delayedInvocation);
            }
            executed = true;
        } else {
            auto immediateQueueSize = immediateQueue.size_approx();
            if (immediateQueueSize > 0) {
                if (immediateQueueSize > immediateQueueBufferSize) {
                    delete[] immediateQueueBuffer;
                    immediateQueueBuffer = new Executable[immediateQueueSize];
                    immediateQueueBufferSize = immediateQueueSize;
                }
                auto numDequeued = immediateQueue.try_dequeue_bulk(
                        immediateQueueToken,
                        immediateQueueBuffer,
                        immediateQueueBufferSize);
                for (size_t i = 0; i < numDequeued; ++ i) {
                    immediateQueueBuffer[i]();
                }
                executed = numDequeued > 0;
            }
        }
    } while (executed);
}

size_t EventLoop::getDelayedQueueSize() const {
    lock_guard guard(delayedQueueLock);
    return delayedQueue.size();
}

DelayedInvocationPtr EventLoop::popDelayedQueue() {
    lock_guard guard(delayedQueueLock);
    if (delayedQueue.empty()) {
        return nullptr;
    }
    auto top = delayedQueue.top();
    if (!top->shouldExecute()) {
        return nullptr;
    }
    delayedQueue.pop();
    return top;
}

}
