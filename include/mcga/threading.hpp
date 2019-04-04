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

namespace mcga::threading {

using Worker = constructs::WorkerConstruct<processors::FunctionProcessor>;
using WorkerThread = constructs::WorkerThreadConstruct<Worker>;
using WorkerThreadPool = constructs::WorkerThreadPoolConstruct<WorkerThread>;
using EventLoop = constructs::EventLoopConstruct<processors::FunctionProcessor>;
using EventLoopThread = constructs::EventLoopThreadConstruct<EventLoop>;
using EventLoopThreadPool = constructs::EventLoopThreadPoolConstruct<EventLoopThread>;

}  // namespace mcga::threading
