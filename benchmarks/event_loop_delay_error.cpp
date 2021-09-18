#include <algorithm>
#include <iostream>

#include <mcga/threading.hpp>

#ifdef LINK_EVPP
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#endif

#include "benchmark_utils.hpp"

using mcga::threading::EventLoopThread;

int main(int argc, char** argv) {
    constexpr int kNumSamplesDefault = 10000;
    int numSamples = kNumSamplesDefault;
    if (argc > 1) {
        numSamples = std::stoi(argv[1]);
    }

#ifdef LINK_EVPP
    DurationTracker evppTracker;

    evpp::EventLoopThread evppLoop;
    evppLoop.Start(true);

    for (int i = 0; i < numSamples; ++i) {
        Stopwatch watch;
        bool done = false;
        evppLoop.loop()->RunAfter(3, [&evppTracker, watch, &done]() {
            watch.track(&evppTracker, std::chrono::milliseconds{3});
            done = true;
        });
        while (!done) {
            std::this_thread::yield();
        }
    }

    evppLoop.Stop(true);

    evppTracker.organize();

    std::cout << "EVPP event loop:\n";
    std::cout << "\tNumber of samples: " << numSamples << "\n";
    std::cout << "\tMinimum error: " << evppTracker.min() << ", "
              << "Maximum error: " << evppTracker.max() << "\n\n";
    std::cout << "\t50%: " << evppTracker.percent(50) << "\n";
    std::cout << "\t75%: " << evppTracker.percent(75) << "\n";
    std::cout << "\t90%: " << evppTracker.percent(90) << "\n";
    std::cout << "\t99%: " << evppTracker.percent(99) << "\n";
#endif

    DurationTracker tracker;

    EventLoopThread loop;
    loop.start();

    for (int i = 0; i < numSamples; ++i) {
        Stopwatch watch;
        bool done = false;
        loop.enqueueDelayed(
          [&tracker, watch, &done]() {
              watch.track(&tracker, std::chrono::milliseconds{3});
              done = true;
          },
          std::chrono::milliseconds{3});
        while (!done) {
            std::this_thread::yield();
        }
    }

    loop.stop();

    tracker.organize();

    std::cout << "\n";
    std::cout << "Own event loop:\n";
    std::cout << "\tNumber of samples: " << numSamples << "\n";
    std::cout << "\tMinimum error: " << tracker.min() << ", "
              << "Maximum error: " << tracker.max() << "\n\n";
    std::cout << "\t50%: " << tracker.percent(50) << "\n";
    std::cout << "\t75%: " << tracker.percent(75) << "\n";
    std::cout << "\t90%: " << tracker.percent(90) << "\n";
    std::cout << "\t99%: " << tracker.percent(99) << "\n";
    return 0;
}
