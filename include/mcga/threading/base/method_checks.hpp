#pragma once

namespace mcga::threading::base {

template<class Processor, class I, class D>
class EventLoop;

template<class T, class Signature>
class HasExecuteTaskImpl {
    using yes = char;
    using no = char[2];

    template<class U, U /*unused*/>
    struct TypeCheck;

    template<class U>
    static yes& checkType(TypeCheck<Signature, &U::executeTask>* /*unused*/) {
    }

    template<class /*unused*/>
    static no& checkType(...) {  // NOLINT(cert-dcl50-cpp)
    }

  public:
    static constexpr bool value = sizeof(checkType<T>(nullptr)) == sizeof(yes);
};

template<class T, class Signature>
constexpr bool hasExecuteTask = HasExecuteTaskImpl<T, Signature>::value;

template<class T>
constexpr bool hasExecuteTaskSimple
  = hasExecuteTask<
      T,
      void (T::*)(
        typename T::
          Task&)> || hasExecuteTask<T, void (T::*)(const typename T::Task&)> || hasExecuteTask<T, void (T::*)(typename T::Task)>;

template<class T, class Enqueuer>
constexpr bool hasExecuteTaskWithEnqueuer
  = hasExecuteTask<
      T,
      void (T::*)(
        typename T::Task&,
        Enqueuer*)> || hasExecuteTask<T, void (T::*)(const typename T::Task&, Enqueuer*)> || hasExecuteTask<T, void (T::*)(typename T::Task, Enqueuer*)>;

}  // namespace mcga::threading::base
