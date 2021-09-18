#include <iostream>
#include <string>
#include <vector>

#include <mcga/threading.hpp>

#ifdef LINK_EVPP
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/event_loop_thread_pool.h>
#endif

#include "benchmark_utils.hpp"

using mcga::threading::EventLoopThread;
using mcga::threading::EventLoopThreadPool;
using mcga::threading::SPEventLoopThread;
using mcga::threading::SPEventLoopThreadPool;
using mcga::threading::StatefulEventLoopThread;
using mcga::threading::StatefulEventLoopThreadPool;
using mcga::threading::StatefulSPEventLoopThread;
using mcga::threading::StatefulSPEventLoopThreadPool;
using mcga::threading::StatelessEventLoopThread;
using mcga::threading::StatelessEventLoopThreadPool;
using mcga::threading::StatelessSPEventLoopThread;
using mcga::threading::StatelessSPEventLoopThreadPool;

int tasksExecuted = 0;
void task() {
    tasksExecuted += 1;
}

std::atomic_int atomicTasksExecuted = 0;
void atomicTask() {
    atomicTasksExecuted += 1;
}

template<class Thread, class Task>
std::chrono::nanoseconds
  sampleDuration(int numSamples, const Task& task, Thread& th) {
    tasksExecuted = 0;
    atomicTasksExecuted = 0;

#ifdef LINK_EVPP
    if constexpr (std::is_same_v<
                    evpp::EventLoopThreadPool,
                    Thread> || std::is_same_v<evpp::EventLoopThread, Thread>) {
        th.Start(true);
    } else
#endif
        th.start();
    Stopwatch watch;
    for (int i = 0; i < numSamples; ++i) {
#ifdef LINK_EVPP
        if constexpr (std::is_same_v<evpp::EventLoopThreadPool, Thread>) {
            th.GetNextLoop()->QueueInLoop(task);
        } else if constexpr (std::is_same_v<evpp::EventLoopThread, Thread>) {
            th.loop()->QueueInLoop(task);
        } else
#endif
            th.enqueue(task);
    }
    while (tasksExecuted != numSamples && atomicTasksExecuted != numSamples) {
        std::this_thread::yield();
    }
    auto totalDuration = watch.get();

#ifdef LINK_EVPP
    if constexpr (std::is_same_v<
                    evpp::EventLoopThreadPool,
                    Thread> || std::is_same_v<evpp::EventLoopThread, Thread>) {
        th.Stop(true);
    } else
#endif
        th.stop();
    return totalDuration;
}

int main(int argc, char** argv) {
    constexpr int kNumSamplesDefault = 10000000;
    int numSamples = kNumSamplesDefault;
    if (argc > 1) {
        numSamples = std::stoi(argv[1]);
    }

#ifdef LINK_EVPP
    evpp::EventLoopThread evppEventLoop1;
    evpp::EventLoopThread evppEventLoop2;
#endif
    EventLoopThread eventLoop;
    StatelessEventLoopThread statelessEventLoop;
    SPEventLoopThread spEventLoop;
    StatelessSPEventLoopThread spStatelessEventLoop;

#ifdef LINK_EVPP
    evpp::EventLoopThreadPool evppEventLoopPool1(
      nullptr, std::thread::hardware_concurrency());
    evpp::EventLoopThreadPool evppEventLoopPool2(
      nullptr, std::thread::hardware_concurrency());
#endif
    EventLoopThreadPool eventLoopPool;
    StatelessEventLoopThreadPool statelessEventLoopPool;
    SPEventLoopThreadPool spEventLoopPool;
    StatelessSPEventLoopThreadPool spStatelessEventLoopPool;

    int capture = 1;
    std::vector<int> capture2(30, 0);
    double capture3 = 3.14;
    auto capturingLambda
      = [&capture, capture2, &capture3] { tasksExecuted += capture; };
    auto atomicCapturingLambda
      = [&capture, capture2, &capture3] { atomicTasksExecuted += capture; };

    auto parameterLambda =
      [](int& capture, std::vector<int> /*unused*/, const double& /*unused*/) {
          tasksExecuted += capture;
      };

    auto atomicParameterLambda =
      [](int& capture, std::vector<int> /*unused*/, const double& /*unused*/) {
          atomicTasksExecuted += capture;
      };

    StatefulEventLoopThread<int&, std::vector<int>, const double&>
      statefulEventLoop(capture, capture2, capture3);
    StatefulEventLoopThreadPool<int&, std::vector<int>, const double&>
      statefulEventLoopPool(capture, capture2, capture3);
    StatefulSPEventLoopThread<int&, std::vector<int>, const double&>
      spStatefulEventLoop(capture, capture2, capture3);
    StatefulSPEventLoopThreadPool<int&, std::vector<int>, const double&>
      spStatefulEventLoopPool(capture, capture2, capture3);

    std::cout << "Non-capturing (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    std::cout << "\tEVPP EventLoop:                       "
              << sampleDuration(numSamples, task, evppEventLoop1) << "\n";
#endif
    std::cout << "\tEventLoop:                            "
              << sampleDuration(numSamples, task, eventLoop) << "\n";
    std::cout << "\tStatelessEventLoop:                   "
              << sampleDuration(numSamples, task, statelessEventLoop) << "\n";
    std::cout << "\tSPEventLoop:                          "
              << sampleDuration(numSamples, task, spEventLoop) << "\n";
    std::cout << "\tStatelessSPEventLoop:                 "
              << sampleDuration(numSamples, task, spStatelessEventLoop) << "\n";
    std::cout << "\n";
#ifdef LINK_EVPP
    std::cout << "\tEVPP EventLoopPool:                   "
              << sampleDuration(numSamples, atomicTask, evppEventLoopPool1)
              << "\n";
#endif
    std::cout << "\tEventLoopPool:                        "
              << sampleDuration(numSamples, atomicTask, eventLoopPool) << "\n";
    std::cout << "\tStatelessEventLoopPool:               "
              << sampleDuration(numSamples, atomicTask, statelessEventLoopPool)
              << "\n";
    std::cout << "\tSPEventLoopPool:                      "
              << sampleDuration(numSamples, atomicTask, spEventLoopPool)
              << "\n";
    std::cout << "\tStatelessSPEventLoopPool:             "
              << sampleDuration(
                   numSamples, atomicTask, spStatelessEventLoopPool)
              << "\n";

    std::cout << "\n\n";

    std::cout << "Capturing (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    std::cout << "\tEVPP EventLoop:                       "
              << sampleDuration(numSamples, capturingLambda, evppEventLoop2)
              << "\n";
#endif
    std::cout << "\tEventLoop:                            "
              << sampleDuration(numSamples, capturingLambda, eventLoop) << "\n";
    std::cout << "\tStatefulEventLoop:                    "
              << sampleDuration(numSamples, parameterLambda, statefulEventLoop)
              << "\n";
    std::cout << "\tSPEventLoop:                          "
              << sampleDuration(numSamples, capturingLambda, spEventLoop)
              << "\n";
    std::cout << "\tStatefulSPEventLoop:                  "
              << sampleDuration(
                   numSamples, parameterLambda, spStatefulEventLoop)
              << "\n";
    std::cout << "\n";
#ifdef LINK_EVPP
    std::cout << "\tEVPP EventLoopPool:                   "
              << sampleDuration(
                   numSamples, atomicCapturingLambda, evppEventLoopPool2)
              << "\n";
#endif
    std::cout << "\tEventLoopPool:                        "
              << sampleDuration(
                   numSamples, atomicCapturingLambda, eventLoopPool)
              << "\n";
    std::cout << "\tStatefulEventLoopPool:                "
              << sampleDuration(
                   numSamples, atomicParameterLambda, statefulEventLoopPool)
              << "\n";
    std::cout << "\tSPEventLoopPool:                      "
              << sampleDuration(
                   numSamples, atomicCapturingLambda, spEventLoopPool)
              << "\n";
    std::cout << "\tStatefulSPEventLoopPool:              "
              << sampleDuration(
                   numSamples, atomicParameterLambda, spStatefulEventLoopPool)
              << "\n";

    return 0;
}
