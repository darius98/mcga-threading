#pragma once

// Constructs
#include <mcga/threading/constructs.hpp>

// Processors
#include <mcga/threading/processors/dispatcher_processor.hpp>
#include <mcga/threading/processors/function_processor.hpp>
#include <mcga/threading/processors/object_processor.hpp>
#include <mcga/threading/processors/stateful_function_processor.hpp>
#include <mcga/threading/processors/stateless_function_processor.hpp>

#define MCGA_THREADING_DEFINE_CONSTRUCT_INTERNAL(                              \
  T_DEF, PROCESSOR, PREFIX, CONSTRUCT)                                         \
    T_DEF using PREFIX##CONSTRUCT                                              \
      = mcga::threading::constructs::CONSTRUCT##Construct<PROCESSOR>;

#define MCGA_THREADING_DEFINE_CONSTRUCTS_INTERNAL(T_DEF, PROCESSOR, PREFIX)    \
    MCGA_THREADING_DEFINE_CONSTRUCT_INTERNAL(                                  \
      T_DEF, PROCESSOR, PREFIX, EventLoopThread);                              \
    MCGA_THREADING_DEFINE_CONSTRUCT_INTERNAL(                                  \
      T_DEF, PROCESSOR, PREFIX, EventLoopThreadPool);                          \
    MCGA_THREADING_DEFINE_CONSTRUCT_INTERNAL(                                  \
      T_DEF, PROCESSOR, PREFIX, SPEventLoopThread);                            \
    MCGA_THREADING_DEFINE_CONSTRUCT_INTERNAL(                                  \
      T_DEF, PROCESSOR, PREFIX, SPEventLoopThreadPool);

#define MCGA_THREADING_DEFINE_CONSTRUCTS(PROCESSOR, PREFIX)                    \
    MCGA_THREADING_DEFINE_CONSTRUCTS_INTERNAL(, PROCESSOR, PREFIX);

#define MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(PROCESSOR, PREFIX)           \
    MCGA_THREADING_DEFINE_CONSTRUCTS_INTERNAL(                                 \
      template<class... T>, PROCESSOR<T...>, PREFIX);

namespace mcga::threading {

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::FunctionProcessor, );

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::ObjectProcessor, Object);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::StatefulFunctionProcessor,
                                          Stateful);

MCGA_THREADING_DEFINE_CONSTRUCTS(processors::StatelessFunctionProcessor,
                                 Stateless);

MCGA_THREADING_DEFINE_TEMPLATE_CONSTRUCTS(processors::DispatcherProcessor,
                                          Dispatcher);

}  // namespace mcga::threading
