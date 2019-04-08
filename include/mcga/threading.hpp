#pragma once

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/base/worker.hpp>

// Constructs
#include <mcga/threading/constructs/event_loop_thread_construct.hpp>
#include <mcga/threading/constructs/event_loop_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_thread_pool_construct.hpp>
#include <mcga/threading/constructs/worker_thread_construct.hpp>

// Processors
#include <mcga/threading/processors/dispatcher_processor.hpp>
#include <mcga/threading/processors/function_processor.hpp>
#include <mcga/threading/processors/object_processor.hpp>
#include <mcga/threading/processors/stateful_function_processor.hpp>
#include <mcga/threading/processors/stateless_function_processor.hpp>

#define MCGA_THREADING_DEFINE_CONSTRUCT(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, PROD, TYPE)        \
    TEMPLATE_START                                                                                          \
    using PREFIX##PROD##TYPE##Thread                                                                        \
        = mcga::threading::constructs::TYPE##ThreadConstruct                                                \
                <mcga::threading::base::PROD##TYPE<PROCESSOR TEMPLATE_END>>;                                \
                                                                                                            \
    TEMPLATE_START                                                                                          \
    using PREFIX##PROD##TYPE##ThreadPool                                                                    \
        = mcga::threading::constructs::TYPE##ThreadPoolConstruct                                            \
                <PREFIX##PROD##TYPE##Thread TEMPLATE_END>;

#define MCGA_THREADING_DEFINE_CONSTRUCT_TYPES(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, PROD)        \
    MCGA_THREADING_DEFINE_CONSTRUCT(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, PROD, EventLoop);      \
    MCGA_THREADING_DEFINE_CONSTRUCT(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, PROD, Worker);

#define MCGA_THREADING_DEFINE_CONSTRUCT_PRODS(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX)              \
    MCGA_THREADING_DEFINE_CONSTRUCT_TYPES(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, );               \
    MCGA_THREADING_DEFINE_CONSTRUCT_TYPES(TEMPLATE_START, TEMPLATE_END, PROCESSOR, PREFIX, SingleProducer);

#define MCGA_THREADING_DEFINE_CONSTRUCTS(PROCESSOR, PREFIX)                                                 \
    MCGA_THREADING_DEFINE_CONSTRUCT_PRODS( , , PROCESSOR, PREFIX);

#define MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(PROCESSOR, PREFIX)                                        \
    MCGA_THREADING_DEFINE_CONSTRUCT_PRODS(template<class... T>, <T...>, PROCESSOR, PREFIX);

namespace mcga::threading {

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::FunctionProcessor, );

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::ObjectProcessor, Object);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::StatefulFunctionProcessor, Stateful);

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::StatelessFunctionProcessor, Stateless);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::DispatcherProcessor, Dispatcher);

}  // namespace mcga::threading
