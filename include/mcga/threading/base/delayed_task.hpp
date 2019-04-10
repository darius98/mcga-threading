#pragma once

#include <atomic>
#include <chrono>
#include <memory>

namespace mcga::threading::base {

template<class Task>
class DelayedTask {
 public:
    using Delay = std::chrono::nanoseconds;

    bool cancel() {
        return cancelled.exchange(true);
    }

 private:
    using Clock = std::chrono::steady_clock;
    using DelayedTaskPtr = std::shared_ptr<DelayedTask>;

    class MakeSharedEnabler : public DelayedTask {
     public:
        MakeSharedEnabler(Task task, const Delay& delay, bool isRepeated)
                : DelayedTask(std::move(task), delay, isRepeated) {}
    };

    struct Compare {
        inline bool operator()(
                const DelayedTaskPtr& a, const DelayedTaskPtr& b) const {
            return a->timePoint > b->timePoint;
        }
    };

    static DelayedTaskPtr delayed(Task task, const Delay& delay) {
        return std::make_shared<MakeSharedEnabler>(
                std::move(task), delay, false);
    }

    static DelayedTaskPtr interval(Task task, const Delay& delay) {
        return std::make_shared<MakeSharedEnabler>(
                std::move(task), delay, true);
    }

    DelayedTask(Task task, const Delay& delay, bool isRepeated):
            task(std::move(task)), delay(delay), isRepeated(isRepeated) {
        setTimePoint();
    }

    bool isCancelled() const {
        return cancelled;
    }

    bool isInterval() const {
        return isRepeated;
    }

    bool shouldExecute() const {
        return timePoint <= Clock::now();
    }

    void setTimePoint() {
        timePoint = Clock::now()
                    + std::chrono::duration_cast<Clock::duration>(delay);
    }

    Task task;
    Delay delay;
    Clock::time_point timePoint;
    bool isRepeated;
    std::atomic_bool cancelled = false;

 template<class Processor> friend class DelayedQueueWrapper;
};

} // namespace mcga::threading::base
