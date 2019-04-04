#pragma once

#include <mcga/threading/base/thread_pool_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class WorkerThreadPoolConstruct : public base::ThreadPoolWrapper<W> {
 public:
    using Object = typename W::Object;

    using base::ThreadPoolWrapper<W>::ThreadPoolWrapper;

    DISALLOW_COPY_AND_MOVE(WorkerThreadPoolConstruct);

    ~WorkerThreadPoolConstruct() = default;

    void enqueue(const Object& func) {
        this->nextThread()->enqueue(func);
    }

    void enqueue(Object&& func) {
        this->nextThread()->enqueue(std::move(func));
    }
};

}  // namespace mcga::threading::constructs
