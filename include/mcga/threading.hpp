#pragma once

// Constructs
#include <mcga/threading/constructs/event_loop_construct.hpp>
#include <mcga/threading/constructs/event_loop_thread_construct.hpp>
#include <mcga/threading/constructs/event_loop_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_construct.hpp>
#include <mcga/threading/constructs/worker_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_thread_construct.hpp>

// Processors
#include <mcga/threading/processors/function_processor.hpp>
#include <mcga/threading/processors/object_processor.hpp>
#include <mcga/threading/processors/stateful_function_processor.hpp>
#include <mcga/threading/processors/stateless_function_processor.hpp>

#define MCGA_THREADING_DEFINE_CONSTRUCTS(PROCESSOR, PREFIX)                    \
    using PREFIX##Worker                                                       \
        = constructs::WorkerConstruct<PROCESSOR>;                              \
                                                                               \
    using PREFIX##WorkerThread                                                 \
        = constructs::WorkerThreadConstruct<PREFIX##Worker>;                   \
                                                                               \
    using PREFIX##WorkerThreadPool                                             \
        = constructs::WorkerThreadPoolConstruct<PREFIX##WorkerThread>;         \
                                                                               \
    using PREFIX##EventLoop                                                    \
        = constructs::EventLoopConstruct<PROCESSOR>;                           \
                                                                               \
    using PREFIX##EventLoopThread                                              \
        = constructs::EventLoopThreadConstruct<PREFIX##EventLoop>;             \
                                                                               \
    using PREFIX##EventLoopThreadPool                                          \
        = constructs::EventLoopThreadPoolConstruct<PREFIX##EventLoopThread>

#define MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(PROCESSOR, PREFIX)           \
    template<class T>                                                          \
    using PREFIX##Worker                                                       \
            = constructs::WorkerConstruct<PROCESSOR<T>>;                       \
                                                                               \
    template<class T>                                                          \
    using PREFIX##WorkerThread                                                 \
        = constructs::WorkerThreadConstruct<PREFIX##Worker<T>>;                \
                                                                               \
    template<class T>                                                          \
    using PREFIX##WorkerThreadPool                                             \
        = constructs::WorkerThreadPoolConstruct<PREFIX##WorkerThread<T>>;      \
                                                                               \
    template<class T>                                                          \
    using PREFIX##EventLoop                                                    \
        = constructs::EventLoopConstruct<PROCESSOR<T>>;                        \
                                                                               \
    template<class T>                                                          \
    using PREFIX##EventLoopThread                                              \
        = constructs::EventLoopThreadConstruct<PREFIX##EventLoop<T>>;          \
                                                                               \
    template<class T>                                                          \
    using PREFIX##EventLoopThreadPool                                          \
        = constructs::EventLoopThreadPoolConstruct<PREFIX##EventLoopThread<T>>

namespace mcga::threading {

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::FunctionProcessor, );

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::ObjectProcessor, Object);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(
        processors::StatefulFunctionProcessor, Stateful);

MCGA_THREADING_DEFINE_CONSTRUCTS(
        processors::StatelessFunctionProcessor, Stateless);

}  // namespace mcga::threading
