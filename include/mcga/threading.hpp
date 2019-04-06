#pragma once

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/base/worker.hpp>

// Constructs
#include <mcga/threading/constructs/event_loop_thread_construct.hpp>
#include <mcga/threading/constructs/event_loop_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_thread_construct.hpp>

// Processors
#include <mcga/threading/processors/function_processor.hpp>
#include <mcga/threading/processors/object_processor.hpp>
#include <mcga/threading/processors/stateful_function_processor.hpp>
#include <mcga/threading/processors/stateless_function_processor.hpp>

#define MCGA_THREADING_DEFINE_CONSTRUCTS(PROCESSOR, PREFIX)                    \
    using PREFIX##WorkerThread                                                 \
        = mcga::threading::constructs::WorkerThreadConstruct                   \
                <mcga::threading::base::Worker<PROCESSOR>>;                    \
                                                                               \
    using PREFIX##WorkerThreadPool                                             \
        = mcga::threading::constructs::WorkerThreadPoolConstruct               \
                <PREFIX##WorkerThread>;                                        \
                                                                               \
    using PREFIX##EventLoopThread                                              \
        = mcga::threading::constructs::EventLoopThreadConstruct                \
                <mcga::threading::base::EventLoop<PROCESSOR>>;                 \
                                                                               \
    using PREFIX##EventLoopThreadPool                                          \
        = mcga::threading::constructs::EventLoopThreadPoolConstruct            \
                <PREFIX##EventLoopThread>

#define MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(PROCESSOR, PREFIX)           \
    template<class... T>                                                       \
    using PREFIX##WorkerThread                                                 \
        = mcga::threading::constructs::WorkerThreadConstruct                   \
                <mcga::threading::base::Worker<PROCESSOR<T...>>>;              \
                                                                               \
    template<class... T>                                                       \
    using PREFIX##WorkerThreadPool                                             \
        = mcga::threading::constructs::WorkerThreadPoolConstruct               \
                <PREFIX##WorkerThread<T...>>;                                  \
                                                                               \
    template<class... T>                                                       \
    using PREFIX##EventLoopThread                                              \
        = mcga::threading::constructs::EventLoopThreadConstruct                \
                <mcga::threading::base::EventLoop<PROCESSOR<T...>>>;           \
                                                                               \
    template<class... T>                                                       \
    using PREFIX##EventLoopThreadPool                                          \
        = mcga::threading::constructs::EventLoopThreadPoolConstruct            \
                <PREFIX##EventLoopThread<T...>>

namespace mcga::threading {

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::FunctionProcessor, );

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::ObjectProcessor, Object);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(
        processors::StatefulFunctionProcessor, Stateful);

MCGA_THREADING_DEFINE_CONSTRUCTS(
        processors::StatelessFunctionProcessor, Stateless);

}  // namespace mcga::threading
