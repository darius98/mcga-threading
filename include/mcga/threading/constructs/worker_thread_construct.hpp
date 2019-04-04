#pragma once

#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class WorkerThreadConstruct : public base::ThreadWrapper<W> {
 public:
    using Object = typename W::Object;

    using base::ThreadWrapper<W>::ThreadWrapper;

    MCGA_THREADING_DISALLOW_COPY_AND_MOVE(WorkerThreadConstruct);

    ~WorkerThreadConstruct() = default;

    void enqueue(const Object& func) {
        this->worker.enqueue(func);
    }

    void enqueue(Object&& func) {
        this->worker.enqueue(std::move(func));
    }
};

}  // namespace mcga::threading::constructs
