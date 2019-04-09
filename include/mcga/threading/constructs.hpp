#pragma once

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/base/thread_pool_wrapper.hpp>
#include <mcga/threading/base/thread_wrapper.hpp>
#include <mcga/threading/base/worker.hpp>

namespace mcga::threading::constructs {

template<class Processor>
using EventLoopThreadConstruct
        = base::EventLoopConstruct<
                base::ThreadWrapper<base::EventLoop<Processor>>
        >;

template<class Processor>
using SingleProducerEventLoopThreadConstruct
        = base::EventLoopConstruct<
                base::ThreadWrapper<base::SingleProducerEventLoop<Processor>>
        >;

template<class Processor>
using EventLoopThreadPoolConstruct
        = base::EventLoopConstruct<
                base::ThreadPoolWrapper<
                        base::EventLoop<Processor>,
                        std::atomic_size_t
                >
        >;

template<class Processor>
using SingleProducerEventLoopThreadPoolConstruct
        = base::EventLoopConstruct<
                base::ThreadPoolWrapper<
                        base::SingleProducerEventLoop<Processor>,
                        std::size_t
                >
        >;

template<class Processor>
using WorkerThreadConstruct
        = base::WorkerConstruct<
                base::ThreadWrapper<base::Worker<Processor>>
        >;

template<class Processor>
using SingleProducerWorkerThreadConstruct
        = base::WorkerConstruct<
                base::ThreadWrapper<base::SingleProducerWorker<Processor>>
        >;

template<class Processor>
using WorkerThreadPoolConstruct
        = base::WorkerConstruct<
                base::ThreadPoolWrapper<
                        base::Worker<Processor>,
                        std::atomic_size_t
                >
        >;

template<class Processor>
using SingleProducerWorkerThreadPoolConstruct
        = base::WorkerConstruct<
                base::ThreadPoolWrapper<
                        base::SingleProducerWorker<Processor>,
                        std::size_t
                >
        >;

}  // namespace mcga::threading::constructs
