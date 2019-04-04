#pragma once

#include <vector>

#include <concurrentqueue.h>

#include "internal/thread_pool_wrapper.hpp"
#include "internal/thread_wrapper.hpp"
#include "internal/worker_construct.hpp"

namespace mcga::threading {

namespace internal {

class WorkerExec {
 public:
    using Object = std::function<void()>;

    static void handleObject(const Object& obj) {
        obj();
    }
};

} // namespace internal

using Worker = internal::WorkerConstruct<internal::WorkerExec>;
using WorkerThread = internal::WorkerThreadConstruct<Worker>;
using WorkerThreadPool = internal::WorkerThreadPoolConstruct<WorkerThread>;

}  // namespace mcga::threading
