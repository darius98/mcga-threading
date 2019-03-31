#pragma once

#include <atomic>
#include <chrono>
#include <memory>

#include "mcga/threading/internal/executable.hpp"

namespace mcga::threading {

class DelayedInvocation {
 public:
    using Clock = std::chrono::steady_clock;
    using Delay = std::chrono::nanoseconds;
    using DelayedInvocationPtr = std::shared_ptr<DelayedInvocation>;

    bool isCancelled() const;

    bool cancel();

    bool isInterval() const;

 private:
    struct Compare {
        inline bool operator()(const DelayedInvocationPtr& a,
                               const DelayedInvocationPtr& b) const {
            return a->timePoint > b->timePoint;
        }
    };

    static DelayedInvocationPtr delayed(Executable executable, Delay delay);
    static DelayedInvocationPtr interval(Executable executable, Delay delay);

    DelayedInvocation(Executable executable, Delay delay, bool isRepeated);

    bool shouldExecute() const;

    void executeIfNotCancelled();

    void setTimePoint();

    Executable executable;
    Delay delay;
    Clock::time_point timePoint;
    bool isRepeated;
    std::atomic_bool cancelled = false;

 friend class EventLoop;
 friend class MakeSharedEnabler;
};
using DelayedInvocationPtr = DelayedInvocation::DelayedInvocationPtr;

}  // namespace mcga::threading
