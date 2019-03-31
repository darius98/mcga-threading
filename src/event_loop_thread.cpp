#include "mcga/threading/event_loop_thread.hpp"

using std::move;

namespace mcga::threading {

void EventLoopThread::enqueue(const Executable& func) {
    worker.enqueue(func);
}

void EventLoopThread::enqueue(Executable&& func) {
    worker.enqueue(move(func));
}

}  // namespace mcga::threading
