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
using mcga::threading::ObjectEventLoopThread;
using mcga::threading::ObjectEventLoopThreadPool;
using std::atomic_int;
using std::cout;
using std::false_type;
using std::stoi;
using std::true_type;
using std::vector;
using std::chrono::nanoseconds;
using std::this_thread::yield;

int tasksExecuted = 0;
void task(int /*obj*/) {
    tasksExecuted += 1;
}
void captureTask(int /*obj*/,
                 int& /*capture1*/,
                 vector<int> /*capture2*/,
                 const double& /*capture3*/) {
    tasksExecuted += 1;
}
void tripleTask(int /*obj1*/, int /*obj2*/, const double* /*obj3*/) {
    tasksExecuted += 1;
}
void tripleTaskCapture(int /*obj1*/,
                       int /*obj2*/,
                       const double* /*obj3*/,
                       int& /*capture1*/,
                       vector<int> /*capture2*/,
                       const double& /*capture3*/) {
    tasksExecuted += 1;
}

atomic_int atomicTasksExecuted = 0;
void atomicTask(int /*obj*/) {
    atomicTasksExecuted += 1;
}
void atomicCaptureTask(int /*obj*/,
                       int& /*capture1*/,
                       vector<int> /*capture2*/,
                       const double& /*capture3*/) {
    atomicTasksExecuted += 1;
}
void tripleAtomicTask(int /*obj1*/, int /*obj2*/, const double* /*obj3*/) {
    atomicTasksExecuted += 1;
}
void atomicTripleTaskCapture(int /*obj1*/,
                             int /*obj2*/,
                             const double* /*obj3*/,
                             int& /*capture1*/,
                             vector<int> /*capture2*/,
                             const double& /*capture3*/) {
    atomicTasksExecuted += 1;
}

template<class Thread>
nanoseconds sampleDuration(int numSamples, Thread& th) {
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
            th.GetNextLoop()->QueueInLoop([i] { atomicTask(i); });
        } else if constexpr (std::is_same_v<evpp::EventLoopThread, Thread>) {
            th.loop()->QueueInLoop([i] { task(i); });
        } else
#endif
          if constexpr (std::is_base_of_v<EventLoopThreadPool, Thread>) {
            th.enqueue([i] { atomicTask(i); });
        } else if constexpr (std::is_base_of_v<EventLoopThread, Thread>) {
            th.enqueue([i] { task(i); });
        } else {
            th.enqueue(i);
        }
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

template<class Thread>
nanoseconds sampleDurationCapture(int numSamples,
                                  Thread& th,
                                  int& capture1,
                                  vector<int> capture2,
                                  const double& capture3) {
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
            th.GetNextLoop()->QueueInLoop([i, &capture1, &capture2, &capture3] {
                atomicCaptureTask(i, capture1, capture2, capture3);
            });
        } else if constexpr (std::is_same_v<evpp::EventLoopThread, Thread>) {
            th.loop()->QueueInLoop([i, &capture1, &capture2, &capture3] {
                captureTask(i, capture1, capture2, capture3);
            });
        } else
#endif
          if constexpr (std::is_base_of_v<EventLoopThreadPool, Thread>) {
            th.enqueue([i, &capture1, &capture2, &capture3] {
                atomicCaptureTask(i, capture1, capture2, capture3);
            });
        } else if constexpr (std::is_base_of_v<EventLoopThread, Thread>) {
            th.enqueue([i, &capture1, &capture2, &capture3] {
                captureTask(i, capture1, capture2, capture3);
            });
        } else {
            th.enqueue(i);
        }
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

template<class Thread>
nanoseconds sampleDurationTriple(int numSamples, Thread& th) {
    tasksExecuted = 0;
    atomicTasksExecuted = 0;

    double d = 3.14;

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
            th.GetNextLoop()->QueueInLoop(
              [i, j = 3 * i, &d] { tripleAtomicTask(i, j, &d); });
        } else if constexpr (std::is_same_v<evpp::EventLoopThread, Thread>) {
            th.loop()->QueueInLoop(
              [i, j = 3 * i, &d] { tripleTask(i, j, &d); });
        } else
#endif
          if constexpr (
            std::is_base_of_v<
              EventLoopThreadPool,
              Thread> ) {
            th.enqueue([i, j = 3 * i, &d] { tripleAtomicTask(i, j, &d); });
        } else if constexpr (
          std::is_base_of_v<
            EventLoopThread,
            Thread> ) {
            th.enqueue([i, j = 3 * i, &d] { tripleTask(i, j, &d); });
        } else {
            th.enqueue({i, 3 * i, &d});
        }
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

template<class Thread>
nanoseconds sampleDurationTripleCapture(int numSamples,
                                        Thread& th,
                                        int& capture1,
                                        vector<int> capture2,
                                        const double& capture3) {
    tasksExecuted = 0;
    atomicTasksExecuted = 0;

    double d = 3.14;

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
            th.GetNextLoop()->QueueInLoop(
              [i, j = 3 * i, &d, &capture1, &capture2, &capture3] {
                  atomicTripleTaskCapture(
                    i, j, &d, capture1, capture2, capture3);
              });
        } else if constexpr (std::is_same_v<evpp::EventLoopThread, Thread>) {
            th.loop()->QueueInLoop(
              [i, j = 3 * i, &d, &capture1, &capture2, &capture3] {
                  tripleTaskCapture(i, j, &d, capture1, capture2, capture3);
              });
        } else
#endif
          if constexpr (
            std::is_base_of_v<
              EventLoopThreadPool,
              Thread>) {
            th.enqueue([i, j = 3 * i, &d, &capture1, &capture2, &capture3] {
                atomicTripleTaskCapture(i, j, &d, capture1, capture2, capture3);
            });
        } else if constexpr (
          std::is_base_of_v<
            EventLoopThread,
            Thread>) {
            th.enqueue([i, j = 3 * i, &d, &capture1, &capture2, &capture3] {
                tripleTaskCapture(i, j, &d, capture1, capture2, capture3);
            });
        } else {
            th.enqueue({i, 3 * i, &d});
        }
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
    evpp::EventLoopThread evppEventLoop3;
    evpp::EventLoopThread evppEventLoop4;
#endif
    EventLoopThread eventLoop;

#ifdef LINK_EVPP
    evpp::EventLoopThreadPool evppEventLoopPool1(
      nullptr, std::thread::hardware_concurrency());
    evpp::EventLoopThreadPool evppEventLoopPool2(
      nullptr, std::thread::hardware_concurrency());
    evpp::EventLoopThreadPool evppEventLoopPool3(
      nullptr, std::thread::hardware_concurrency());
    evpp::EventLoopThreadPool evppEventLoopPool4(
      nullptr, std::thread::hardware_concurrency());
#endif
    EventLoopThreadPool eventLoopPool;

    ObjectEventLoopThread<int> objectEventLoop(task);
    ObjectEventLoopThreadPool<int> objectEventLoopPool(atomicTask);

    cout << "No state, single object (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:         "
         << sampleDuration(numSamples, evppEventLoop1) << "\n";
#endif
    cout << "\tEventLoop:              "
         << sampleDuration(numSamples, eventLoop) << "\n";
    cout << "\tObjectEventLoop:        "
         << sampleDuration(numSamples, objectEventLoop) << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:     "
         << sampleDuration(numSamples, evppEventLoopPool1) << "\n";
#endif
    cout << "\tEventLoopPool:          "
         << sampleDuration(numSamples, eventLoopPool) << "\n";
    cout << "\tObjectEventLoopPool:    "
         << sampleDuration(numSamples, objectEventLoopPool) << "\n";

    cout << "\n\n";

    int capture1 = 0;
    vector<int> capture2(20, 0);
    double capture3 = 3.14;

    ObjectEventLoopThread<int> captureObjectEventLoop(
      [&capture1, &capture2, &capture3](int obj) {
          captureTask(obj, capture1, capture2, capture3);
      });
    ObjectEventLoopThreadPool<int> captureObjectEventLoopPool(
      [&capture1, &capture2, &capture3](int obj) {
          atomicCaptureTask(obj, capture1, capture2, capture3);
      });

    cout << "With state (3x reference capture), single object (" << numSamples
         << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:         "
         << sampleDurationCapture(
              numSamples, evppEventLoop2, capture1, capture2, capture3)
         << "\n";
#endif
    cout << "\tEventLoop:              "
         << sampleDurationCapture(
              numSamples, eventLoop, capture1, capture2, capture3)
         << "\n";
    cout << "\tObjectEventLoop:        "
         << sampleDurationCapture(
              numSamples, objectEventLoop, capture1, capture2, capture3)
         << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:     "
         << sampleDurationCapture(
              numSamples, evppEventLoopPool2, capture1, capture2, capture3)
         << "\n";
#endif
    cout << "\tEventLoopPool:          "
         << sampleDurationCapture(
              numSamples, eventLoopPool, capture1, capture2, capture3)
         << "\n";
    cout << "\tObjectEventLoopPool:    "
         << sampleDurationCapture(
              numSamples, objectEventLoopPool, capture1, capture2, capture3)
         << "\n";

    cout << "\n\n";

    ObjectEventLoopThread<int, int, const double*> tripleObjectEventLoop(
      tripleTask);
    ObjectEventLoopThreadPool<int, int, const double*>
      tripleObjectEventLoopPool(tripleAtomicTask);

    cout << "No state, 3 objects (" << numSamples << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:         "
         << sampleDurationTriple(numSamples, evppEventLoop3) << "\n";
#endif
    cout << "\tEventLoop:              "
         << sampleDurationTriple(numSamples, eventLoop) << "\n";
    cout << "\tObjectEventLoop:        "
         << sampleDurationTriple(numSamples, tripleObjectEventLoop) << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:     "
         << sampleDurationTriple(numSamples, evppEventLoopPool3) << "\n";
#endif
    cout << "\tEventLoopPool:          "
         << sampleDurationTriple(numSamples, eventLoopPool) << "\n";
    cout << "\tObjectEventLoopPool:    "
         << sampleDurationTriple(numSamples, tripleObjectEventLoopPool) << "\n";

    cout << "\n\n";

    ObjectEventLoopThread<int, int, const double*> tripleObjectCaptureEventLoop(
      [&capture1, &capture2, &capture3](
        int obj1, int obj2, const double* obj3) {
          tripleTaskCapture(obj1, obj2, obj3, capture1, capture2, capture3);
      });
    ObjectEventLoopThreadPool<int, int, const double*>
      tripleObjectCaptureEventLoopPool(
        [&capture1, &capture2, &capture3](
          int obj1, int obj2, const double* obj3) {
            atomicTripleTaskCapture(
              obj1, obj2, obj3, capture1, capture2, capture3);
        });

    cout << "With state (3x reference capture), 3 objects (" << numSamples
         << " samples):\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoop:         "
         << sampleDurationTripleCapture(
              numSamples, evppEventLoop4, capture1, capture2, capture3)
         << "\n";
#endif
    cout << "\tEventLoop:              "
         << sampleDurationTripleCapture(
              numSamples, eventLoop, capture1, capture2, capture3)
         << "\n";
    cout << "\tObjectEventLoop:        "
         << sampleDurationTripleCapture(numSamples,
                                        tripleObjectCaptureEventLoop,
                                        capture1,
                                        capture2,
                                        capture3)
         << "\n";
    cout << "\n";
#ifdef LINK_EVPP
    cout << "\tEVPP EventLoopPool:     "
         << sampleDurationTripleCapture(
              numSamples, evppEventLoopPool4, capture1, capture2, capture3)
         << "\n";
#endif
    cout << "\tEventLoopPool:          "
         << sampleDurationTripleCapture(
              numSamples, eventLoopPool, capture1, capture2, capture3)
         << "\n";
    cout << "\tObjectEventLoopPool:    "
         << sampleDurationTripleCapture(numSamples,
                                        tripleObjectCaptureEventLoopPool,
                                        capture1,
                                        capture2,
                                        capture3)
         << "\n";

    return 0;
}
