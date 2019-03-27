#include "mcga/threading/event_loop.hpp"

using std::lock_guard;
using std::move;
using std::size_t;
using std::this_thread::sleep_for;
using namespace std::literals::chrono_literals;

namespace mcga::threading {

size_t EventLoop::size() const {
    return getImmediateQueueSize() + getDelayedQueueSize();
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
    lock_guard guard(immediateQueueLock);
    immediateQueue.push(func);
}

void EventLoop::enqueue(Executable&& func) {
    lock_guard guard(immediateQueueLock);
    immediateQueue.push(move(func));
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
            auto immediateInvocation = popImmediateQueue();
            if (immediateInvocation != nullptr) {
                immediateInvocation();
                executed = true;
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

size_t EventLoop::getImmediateQueueSize() const {
    lock_guard guard(immediateQueueLock);
    return immediateQueue.size();
}

Executable EventLoop::popImmediateQueue() {
    lock_guard guard(immediateQueueLock);
    if (immediateQueue.empty()) {
        return nullptr;
    }
    auto top = immediateQueue.front();
    immediateQueue.pop();
    return top;
}

}
