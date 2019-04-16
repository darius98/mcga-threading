#pragma ide diagnostic ignored "readability-magic-numbers"

#include <algorithm>
#include <iostream>

#include <mcga/threading.hpp>

#ifdef LINK_EVPP
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#endif

#include "benchmark_utils.hpp"

using mcga::threading::EventLoopThread;
using std::cout;
using std::operator""ms;
using std::stoi;
using std::this_thread::yield;

int main(int argc, char** argv) {
    constexpr int kNumSamplesDefault = 10000;
    int numSamples = kNumSamplesDefault;
    if (argc > 1) {
        numSamples = stoi(argv[1]);
    }

#ifdef LINK_EVPP
    DurationTracker evppTracker;

    evpp::EventLoopThread evppLoop;
    evppLoop.Start(true);

    for (int i = 0; i < numSamples; ++i) {
        Stopwatch watch;
        bool done = false;
        evppLoop.loop()->RunAfter(3, [&evppTracker, watch, &done]() {
            watch.track(&evppTracker, 3ms);
            done = true;
        });
        while (!done) {
            yield();
        }
    }

    evppLoop.Stop(true);

    evppTracker.organize();

    cout << "EVPP event loop:\n";
    cout << "\tNumber of samples: " << numSamples << "\n";
    cout << "\tMinimum error: " << evppTracker.min() << ", "
         << "Maximum error: " << evppTracker.max() << "\n\n";
    cout << "\t50%: " << evppTracker.percent(50) << "\n";
    cout << "\t75%: " << evppTracker.percent(75) << "\n";
    cout << "\t90%: " << evppTracker.percent(90) << "\n";
    cout << "\t99%: " << evppTracker.percent(99) << "\n";
#endif

    DurationTracker tracker;

    EventLoopThread loop;
    loop.start();

    for (int i = 0; i < numSamples; ++i) {
        Stopwatch watch;
        bool done = false;
        loop.enqueueDelayed(
          [&tracker, watch, &done]() {
              watch.track(&tracker, 3ms);
              done = true;
          },
          3ms);
        while (!done) {
            yield();
        }
    }

    loop.stop();

    tracker.organize();

    cout << "\n";
    cout << "Own event loop:\n";
    cout << "\tNumber of samples: " << numSamples << "\n";
    cout << "\tMinimum error: " << tracker.min() << ", "
         << "Maximum error: " << tracker.max() << "\n\n";
    cout << "\t50%: " << tracker.percent(50) << "\n";
    cout << "\t75%: " << tracker.percent(75) << "\n";
    cout << "\t90%: " << tracker.percent(90) << "\n";
    cout << "\t99%: " << tracker.percent(99) << "\n";
    return 0;
}
