#pragma once

#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class W>
class WorkerThreadConstruct : public base::ThreadWrapper<W> {
 public:
    using Object = typename W::Object;

    WorkerThreadConstruct() = default;
    ~WorkerThreadConstruct() = default;

    DISALLOW_COPY_AND_MOVE(WorkerThreadConstruct);

    void enqueue(const Object& func) {
        this->worker.enqueue(func);
    }

    void enqueue(Object&& func) {
        this->worker.enqueue(std::move(func));
    }

 private:
    explicit WorkerThreadConstruct(volatile std::atomic_bool* running):
            base::ThreadWrapper<W>(running) {}

 friend class base::ThreadPoolWrapper<WorkerThreadConstruct>;
};

}  // namespace mcga::threading::constructs
