#pragma once

#include <chrono>
#include <iomanip>
#include <ostream>
#include <vector>

inline std::ostream& operator<<(std::ostream& os,
                                const std::chrono::nanoseconds& ns) {
    if (ns.count() > 1000000000) {
        os << std::fixed << std::setprecision(3) << ns.count() * 1.e-9 << "s";
    } else if (ns.count() > 1000000) {
        os << std::fixed << std::setprecision(3) << ns.count() * 1.e-6 << "ms";
    } else if (ns.count() > 1000) {
        os << std::fixed << std::setprecision(3) << ns.count() * 1.e-3 << "us";
    } else {
        os << ns.count() << "ns";
    }
    return os;
}

class DurationTracker {
  public:
    std::vector<std::chrono::nanoseconds> samples;

    void addSample(const std::chrono::nanoseconds& ns) {
        samples.push_back(ns);
    }

    void organize() {
        sort(samples.begin(), samples.end());
    }

    std::chrono::nanoseconds min() const {
        return samples.front();
    }

    std::chrono::nanoseconds max() const {
        return samples.back();
    }

    std::chrono::nanoseconds percent(double p) const {
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
    Stopwatch(): startTime(std::chrono::steady_clock::now()) {
    }

    std::chrono::nanoseconds get() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now() - startTime);
    }

    void track(DurationTracker* tracker,
               std::chrono::nanoseconds expected) const {
        tracker->addSample(get() - expected);
    }

  private:
    std::chrono::steady_clock::time_point startTime;
};
