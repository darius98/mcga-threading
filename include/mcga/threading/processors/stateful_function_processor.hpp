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
        handleObjectImpl(obj, std::make_index_sequence<numArgs>());
    }

 private:
    static constexpr std::size_t numArgs
            = std::tuple_size<std::tuple<Args...>>::value;

    template<std::size_t... S>
    void handleObjectImpl(const Object& obj,
                          std::index_sequence<S...> /*unused*/) {
        obj(std::get<S>(args)...);
    }

    std::tuple<Args...> args;
};

} // namespace mcga::threading::processors
