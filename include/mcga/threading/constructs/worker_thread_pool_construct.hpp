#pragma once

#include <atomic>

#include <mcga/threading/base/thread_pool_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W, class Idx = std::atomic_size_t>
class WorkerThreadPoolConstruct: public base::ThreadPoolWrapper<W, Idx> {
 public:
    using Task = typename W::Task;

    using base::ThreadPoolWrapper<W, Idx>::ThreadPoolWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(WorkerThreadPoolConstruct);

    ~WorkerThreadPoolConstruct() = default;

    void enqueue(Task task) {
        this->nextThread()->enqueue(std::move(task));
    }
};

}  // namespace mcga::threading::constructs
