#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include <mcga/threading/event_loop_thread.hpp>

using namespace mcga::threading;
using namespace std;
using namespace std::chrono;

class DurationTracker {
 public:
    vector<nanoseconds> samples;

    void addSample(const nanoseconds& ns) {
        samples.push_back(ns);
    }

    void organize() {
        sort(samples.begin(), samples.end());
    }

    nanoseconds min() const {
        return samples.front();
    }

    nanoseconds max() const {
        return samples.back();
    }

    nanoseconds percent(double p) const {
        auto index = static_cast<size_t>(p * 0.01 * samples.size());
        if (index >= samples.size() - 1) {
            return max();
        }
        if (index < 1) {
            return min();
        }
        return samples[index + 1];
    }
};

class Stopwatch {
 public:
    Stopwatch(): startTime(steady_clock::now()) {}

    nanoseconds get() const {
        return duration_cast<nanoseconds>(steady_clock::now() - startTime);
    }

    void track(DurationTracker* tracker, nanoseconds expected) const {
        tracker->addSample(get() - expected);
    }
 private:
    steady_clock::time_point startTime;
};

ostream& operator<<(ostream& os, const chrono::nanoseconds& ns) {
    if (ns.count() > 1000000000) {
        os << fixed << setprecision(3) << ns.count() * 1.e-9 << "s";
    } else if (ns.count() > 1000000) {
        os << fixed << setprecision(3) << ns.count() * 1.e-6 << "ms";
    } else if (ns.count() > 1000) {
        os << fixed << setprecision(3) << ns.count() * 1.e-3 << "us";
    } else {
        os << ns.count() << "ns";
    }
    return os;
}

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
        loop.enqueueDelayed([&tracker, watch]() {
            watch.track(&tracker, 3ms);
        }, 3ms);
        while (loop.size() > 0) {
            this_thread::sleep_for(100us);
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
