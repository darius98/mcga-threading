#pragma once

#include <mcga/threading/base/opaque_enqueuer.hpp>

namespace mcga::threading::base {

template<class T, class Signature>
class HasExecuteTaskImpl {
    using yes = char;
    using no = char[2];

    template <class U, U /*unused*/> struct TypeCheck;

    template <class U>
    static yes& checkType(TypeCheck<Signature, &U::executeTask>* /*unused*/) {}

    template <class /*unused*/>
    static no&  checkType(...) {} // NOLINT(cert-dcl50-cpp)

 public:
    static constexpr bool value
            = sizeof(checkType<T>(nullptr)) == sizeof(yes);
};

template<class T, class Signature>
constexpr bool hasExecuteTask = HasExecuteTaskImpl<T, Signature>::value;

template<class T>
constexpr bool hasExecuteTaskSimple
    = hasExecuteTask<T, void (T::*)(typename T::Task&&)>
      || hasExecuteTask<T, void (T::*)(const typename T::Task&)>
      || hasExecuteTask<T, void (T::*)(typename T::Task)>;

template<class T, class Enqueuer>
constexpr bool hasExecuteTaskWithEnqueuer
    = hasExecuteTask<T, void (T::*)(typename T::Task&&, Enqueuer*)>
      || hasExecuteTask<T, void (T::*)(const typename T::Task&, Enqueuer*)>
      || hasExecuteTask<T, void (T::*)(typename T::Task, Enqueuer*)>;

template<class T>
constexpr bool hasExecuteTaskWithWorkerEnqueuer
    = hasExecuteTaskWithEnqueuer<T, OpaqueWorkerEnqueuer<typename T::Task>>;

template<class T>
constexpr bool hasExecuteTaskWithEventLoopEnqueuer
    = hasExecuteTaskWithEnqueuer<T, OpaqueEventLoopEnqueuer<typename T::Task>>;

template<class Processor>
inline void executeTask(
        typename Processor::Task&& task,
        Processor* processor,
        OpaqueWorkerEnqueuer<typename Processor::Task>* enqueuer) {
    if constexpr (hasExecuteTaskWithWorkerEnqueuer<Processor>) {
        processor->executeTask(std::move(task), enqueuer);
    } else {
        processor->executeTask(std::move(task));
    }
}

template<class Processor>
inline void executeTask(
        typename Processor::Task&& task,
        Processor* processor,
        OpaqueEventLoopEnqueuer<typename Processor::Task>* enqueuer) {
    if constexpr (hasExecuteTaskWithWorkerEnqueuer<Processor>
                    || hasExecuteTaskWithEventLoopEnqueuer<Processor>) {
        processor->executeTask(std::move(task), enqueuer);
    } else {
        processor->executeTask(std::move(task));
    }
}

} // namespace mcga::threading::base
