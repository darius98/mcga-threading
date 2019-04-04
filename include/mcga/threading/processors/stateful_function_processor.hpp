#pragma once

#include <functional>
#include <tuple>

namespace mcga::threading::processors {

template<class... Args>
class StatefulFunctionProcessor {
 public:
    using Object = void (*)(Args&&...);

    explicit StatefulFunctionProcessor(Args&&... args):
            args(std::forward<Args>(args)...) {}

    void handleObject(const Object& obj) {
        std::apply(obj, args);
    }

 private:
    std::tuple<Args...> args;
};

} // namespace mcga::threading::processors
