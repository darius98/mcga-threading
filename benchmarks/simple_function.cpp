#pragma ide diagnostic ignored "readability-magic-numbers"
#pragma ide diagnostic ignored "google-readability-braces-around-statements"

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
using mcga::threading::SingleProducerEventLoopThread;
using mcga::threading::SingleProducerEventLoopThreadPool;
using mcga::threading::StatefulEventLoopThread;
using mcga::threading::StatefulEventLoopThreadPool;
using mcga::threading::StatefulSingleProducerEventLoopThread;
using mcga::threading::StatefulSingleProducerEventLoopThreadPool;
using mcga::threading::StatelessEventLoopThread;
using mcga::threading::StatelessEventLoopThreadPool;
using mcga::threading::StatelessSingleProducerEventLoopThread;
using mcga::threading::StatelessSingleProducerEventLoopThreadPool;
using std::atomic_int;
using std::cout;
using std::false_type;
using std::stoi;
using std::true_type;
using std::vector;
using std::chrono::nanoseconds;
using std::this_thread::yield;

int tasksExecuted = 0;
void task() {
    tasksExecuted += 1;
}

atomic_int atomicTasksExecuted = 0;
void atomicTask() {
    atomicTasksExecuted += 1;
}

template<class Thread, class Task>
nanoseconds sampleDuration(int numSamples, const Task& task, Thread& th) {
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
        yield();
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
        numSamples = stoi(argv[1]);
    }

#ifdef LINK_EVPP
    evpp::EventLoopThread evppEventLoop1;
    evpp::EventLoopThread evppEventLoop2;
#endif
    EventLoopThread eventLoop;
    StatelessEventLoopThread statelessEventLoop;
    SingleProducerEventLoopThread spEventLoop;
    StatelessSingleProducerEventLoopThread spStatelessEventLoop;

#ifdef LINK_EVPP
    evpp::EventLoopThreadPool evppEventLoopPool1(
      nullptr, std::thread::hardware_concurrency());
    evpp::EventLoopThreadPool evppEventLoopPool2(
      nullptr, std::thread::hardware_concurrency());
#endif
    EventLoopThreadPool eventLoopPool;
    StatelessEventLoopThreadPool statelessEventLoopPool;
    SingleProducerEventLoopThreadPool spEventLoopPool;
    StatelessSingleProducerEventLoopThreadPool spStatelessEventLoopPool;

    int capture = 1;
    vector<int> capture2(30, 0);
    double capture3 = 3.14;
    auto capturingLambda
      = [&capture, capture2, &capture3] { tasksExecuted += capture; };
    auto atomicCapturingLambda
      = [&capture, capture2, &capture3] { atomicTasksExecuted += capture; };

    auto parameterLambda
      = [](int& capture, vector<int> /*unused*/, const double& /*unused*/) {
            tasksExecuted += capture;
        };

    auto atomicParameterLambda
      = [](int& capture, vector<int> /*unused*/, const double& /*unused*/) {
            atomicTasksExecuted += capture;
        };

    StatefulEventLoopThread<int&, vector<int>, const double&> statefulEventLoop(
      capture, capture2, capture3);
    StatefulEventLoopThreadPool<int&, vector<int>, const double&>
      statefulEventLoopPool(capture, capture2, capture3);
    StatefulSingleProducerEventLoopThread<int&, vector<int>, const double&>
      spStatefulEventLoop(capture, capture2, capture3);
    StatefulSingleProducerEventLoopThreadPool<int&, vector<int>, const double&>
      spStatefulEventLoopPool(capture, capture2, capture3);

    cout << "Non-capturing (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:                       "
         << sampleDuration(numSamples, task, evppEventLoop1) << "\n";
#endif
    cout << "\tEventLoop:                            "
         << sampleDuration(numSamples, task, eventLoop) << "\n";
    cout << "\tStatelessEventLoop:                   "
         << sampleDuration(numSamples, task, statelessEventLoop) << "\n";
    cout << "\tSingleProducerEventLoop:              "
         << sampleDuration(numSamples, task, spEventLoop) << "\n";
    cout << "\tStatelessSingleProducerEventLoop:     "
         << sampleDuration(numSamples, task, spStatelessEventLoop) << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:                   "
         << sampleDuration(numSamples, atomicTask, evppEventLoopPool1) << "\n";
#endif
    cout << "\tEventLoopPool:                        "
         << sampleDuration(numSamples, atomicTask, eventLoopPool) << "\n";
    cout << "\tStatelessEventLoopPool:               "
         << sampleDuration(numSamples, atomicTask, statelessEventLoopPool)
         << "\n";
    cout << "\tSingleProducerEventLoopPool:          "
         << sampleDuration(numSamples, atomicTask, spEventLoopPool) << "\n";
    cout << "\tStatelessSingleProducerEventLoopPool: "
         << sampleDuration(numSamples, atomicTask, spStatelessEventLoopPool)
         << "\n";

    cout << "\n\n";

    cout << "Capturing (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:                       "
         << sampleDuration(numSamples, capturingLambda, evppEventLoop2) << "\n";
#endif
    cout << "\tEventLoop:                            "
         << sampleDuration(numSamples, capturingLambda, eventLoop) << "\n";
    cout << "\tStatefulEventLoop:                    "
         << sampleDuration(numSamples, parameterLambda, statefulEventLoop)
         << "\n";
    cout << "\tSingleProducerEventLoop:              "
         << sampleDuration(numSamples, capturingLambda, spEventLoop) << "\n";
    cout << "\tStatefulSingleProducerEventLoop:      "
         << sampleDuration(numSamples, parameterLambda, spStatefulEventLoop)
         << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:                   "
         << sampleDuration(
              numSamples, atomicCapturingLambda, evppEventLoopPool2)
         << "\n";
#endif
    cout << "\tEventLoopPool:                        "
         << sampleDuration(numSamples, atomicCapturingLambda, eventLoopPool)
         << "\n";
    cout << "\tStatefulEventLoopPool:                "
         << sampleDuration(
              numSamples, atomicParameterLambda, statefulEventLoopPool)
         << "\n";
    cout << "\tSingleProducerEventLoopPool:          "
         << sampleDuration(numSamples, atomicCapturingLambda, spEventLoopPool)
         << "\n";
    cout << "\tStatefulSingleProducerEventLoopPool:  "
         << sampleDuration(
              numSamples, atomicParameterLambda, spStatefulEventLoopPool)
         << "\n";

    return 0;
}
