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

    struct Compare {
        inline bool operator()(const DelayedTaskPtr& a,
                               const DelayedTaskPtr& b) const {
            return a->timePoint > b->timePoint;
        }
    };

    class MakeSharedEnabler;
    static DelayedTaskPtr delayed(Task task, const Delay& delay);
    static DelayedTaskPtr interval(Task task, const Delay& delay);

    DelayedTask(Task task, const Delay& delay, bool isRepeated)
            : task(std::move(task)), delay(delay), isRepeated(isRepeated) {
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
        timePoint
          = Clock::now() + std::chrono::duration_cast<Clock::duration>(delay);
    }

    Task task;
    Delay delay;
    Clock::time_point timePoint;
    bool isRepeated;
    std::atomic_bool cancelled = false;

    template<class Processor>
    friend class DelayedQueueWrapper;
};

template<class Task>
class DelayedTask<Task>::MakeSharedEnabler : public DelayedTask<Task> {
  public:
    MakeSharedEnabler(Task task, const Delay& delay, bool isRepeated)
            : DelayedTask(std::move(task), delay, isRepeated) {
    }
};

template<class Task>
auto DelayedTask<Task>::delayed(Task task, const Delay& delay)
  -> DelayedTaskPtr {
    return std::make_shared<MakeSharedEnabler>(std::move(task), delay, false);
}

template<class Task>
auto DelayedTask<Task>::interval(Task task, const Delay& delay)
  -> DelayedTaskPtr {
    return std::make_shared<MakeSharedEnabler>(std::move(task), delay, true);
}

}  // namespace mcga::threading::base
