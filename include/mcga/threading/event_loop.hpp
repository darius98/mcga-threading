#pragma once

#include <functional>

#include "internal/event_loop_construct.hpp"

namespace mcga::threading {

namespace internal {

class WorkerExec2 {
 public:
    using Object = std::function<void()>;

    static void handleObject(const Object& obj) {
        obj();
    }
};

}  // namespace internal

using EventLoop = internal::EventLoopConstruct<internal::WorkerExec2>;
using EventLoopThread = internal::EventLoopThreadConstruct<EventLoop>;
using EventLoopThreadPool = internal::EventLoopThreadPoolConstruct<EventLoopThread>;

}  // namespace mcga::threading
