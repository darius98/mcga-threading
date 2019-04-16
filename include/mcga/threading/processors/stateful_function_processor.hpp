#pragma once

#include <functional>
#include <tuple>

namespace mcga::threading::processors {

template<class... Args>
class StatefulFunctionProcessor {
  public:
    using Task = void (*)(Args...);

    explicit StatefulFunctionProcessor(Args... args)
            : args(std::forward<Args>(args)...) {
    }

    void executeTask(const Task& task) {
        std::apply(task, args);
    }

  private:
    std::tuple<Args...> args;
};

}  // namespace mcga::threading::processors
