#include "mcga/threading/event_loop_thread.hpp"

using std::lock_guard;
using std::move;
using std::size_t;
using std::thread;
using std::this_thread::sleep_for;
using namespace std::literals::chrono_literals;

namespace mcga::threading {

EventLoopThread::~EventLoopThread() {
    stop();
}

size_t EventLoopThread::size() const {
    return loop.size();
}

void EventLoopThread::start() {
    lock_guard guard(startStopMutex);
    if (isStarted) {
        return;
    }
    loopThread = thread([this]() {
        isStarted = true;
        loop.start();
    });
    while (!isStarted) {
        sleep_for(1ns);
    }
}

void EventLoopThread::stop() {
    lock_guard guard(startStopMutex);
    if (!isStarted) {
        return;
    }
    loop.stop();
    loopThread.join();
    isStarted = false;
}

void EventLoopThread::enqueue(const Executable& func) {
    loop.enqueue(func);
}

void EventLoopThread::enqueue(Executable&& func) {
    loop.enqueue(move(func));
}

}
