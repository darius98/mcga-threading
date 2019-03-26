#include "mcga/threading/event_loop.hpp"

using std::lock_guard;
using std::move;
using std::scoped_lock;
using std::size_t;
using std::this_thread::sleep_for;

namespace mcga::threading {

size_t EventLoop::size() const {
    scoped_lock guard(immediateQueueLock, delayedQueueLock);
    return immediateQueue.size() + delayedQueue.size();
}

void EventLoop::start() {
    running = true;
    while (running) {
        executePending();
        sleep_for(tick);
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
