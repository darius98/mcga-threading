#pragma ide diagnostic ignored "readability-magic-numbers"

#include <algorithm>
#include <iostream>

#include <mcga/threading.hpp>

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

    DurationTracker tracker;

    EventLoopThread loop;
    loop.start();

    for (int i = 0; i < numSamples; ++ i) {
        Stopwatch watch;
        bool done = false;
        loop.enqueueDelayed([&tracker, watch, &done]() {
            watch.track(&tracker, 3ms);
            done = true;
        }, 3ms);
        while (!done) {
            yield();
        }
    }

    loop.stop();

    tracker.organize();

    cout << "Number of samples: " << numSamples << "\n";
    cout << "Minimum error: " << tracker.min() << ", "
         << "Maximum error: " << tracker.max() << "\n\n";
    cout << "50%: " << tracker.percent(50) << "\n";
    cout << "75%: " << tracker.percent(75) << "\n";
    cout << "90%: " << tracker.percent(90) << "\n";
    cout << "99%: " << tracker.percent(99) << "\n";
    return 0;
}
