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
    return isRepeated;
}

class MakeSharedEnabler: public DelayedInvocation {
 public:
    MakeSharedEnabler(Executable executable, Delay delay, bool isRepeated)
            : DelayedInvocation(move(executable), delay, isRepeated) {}
};

DelayedInvocationPtr DelayedInvocation::delayed(Executable func, Delay delay) {
    return make_shared<MakeSharedEnabler>(move(func), delay, false);
}

DelayedInvocationPtr DelayedInvocation::interval(Executable func, Delay delay) {
    return make_shared<MakeSharedEnabler>(move(func), delay, true);
}

DelayedInvocation::DelayedInvocation(
        Executable executable, Delay delay, bool isRepeated)
        : executable(move(executable)), delay(delay), isRepeated(isRepeated) {
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

}  // namespace mcga::threading
