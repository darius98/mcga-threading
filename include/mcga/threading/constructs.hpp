#pragma once

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/base/thread_pool_wrapper.hpp>
#include <mcga/threading/base/thread_wrapper.hpp>
#include <mcga/threading/base/worker.hpp>

namespace mcga::threading::constructs {

template<class W>
using EventLoopThreadConstruct
        = base::EventLoopConstruct<base::ThreadWrapper<W>>;

template<class W, class Idx = std::atomic_size_t>
using EventLoopThreadPoolConstruct
        = base::EventLoopConstruct<base::ThreadPoolWrapper<W, Idx>>;

template<class W>
using WorkerThreadConstruct = base::WorkerConstruct<base::ThreadWrapper<W>>;

template<class W, class Idx = std::atomic_size_t>
using WorkerThreadPoolConstruct
        = base::WorkerConstruct<base::ThreadPoolWrapper<W, Idx>>;

}  // namespace mcga::threading::constructs
