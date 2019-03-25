#include "mcga/threading/delayed_invocation.hpp"

using std::make_shared;
using std::move;
using std::chrono::duration_cast;

namespace mcga::threading {

bool DelayedInvocation::isCancelled() const {
    return cancelled;
}

bool DelayedInvocation::cancel() {
    return cancelled.exchange(true);
}

bool DelayedInvocation::isInterval() const {
    return interval;
}

class MakeSharedEnabler: public DelayedInvocation {
 public:
    MakeSharedEnabler(Executable executable, Delay _delay, bool _interval)
            : DelayedInvocation(move(executable), _delay, _interval) {}
};

DelayedInvocationPtr DelayedInvocation::Delayed(Executable func, Delay delay) {
    return make_shared<MakeSharedEnabler>(move(func), delay, false);
}

DelayedInvocationPtr DelayedInvocation::Interval(Executable func, Delay delay) {
    return make_shared<MakeSharedEnabler>(move(func), delay, true);
}

DelayedInvocation::DelayedInvocation(
        Executable executable, Delay _delay, bool _interval)
        : executable(move(executable)), delay(_delay), interval(_interval) {
    setTimePoint();
}

bool DelayedInvocation::shouldExecute() const {
    return timePoint <= Clock::now();
}

void DelayedInvocation::executeIfNotCancelled() {
    if (!cancelled) {
        executable();
    }
}

void DelayedInvocation::setTimePoint() {
    timePoint = Clock::now() + duration_cast<Clock::duration>(delay);
}

}
