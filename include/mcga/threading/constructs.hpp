#pragma once

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/base/thread_pool_wrapper.hpp>
#include <mcga/threading/base/thread_wrapper.hpp>

namespace mcga::threading::constructs {

template<class Processor>
using EventLoopThreadConstruct
  = base::EventLoopConstruct<base::ThreadWrapper<base::EventLoop<Processor>>>;

template<class Processor>
using SPEventLoopThreadConstruct
  = base::EventLoopConstruct<base::ThreadWrapper<base::SPEventLoop<Processor>>>;

template<class Processor>
using EventLoopThreadPoolConstruct = base::EventLoopConstruct<
  base::ThreadPoolWrapper<base::EventLoop<Processor>, std::atomic_size_t>>;

template<class Processor>
using SPEventLoopThreadPoolConstruct = base::EventLoopConstruct<
  base::ThreadPoolWrapper<base::SPEventLoop<Processor>, std::size_t>>;

}  // namespace mcga::threading::constructs
