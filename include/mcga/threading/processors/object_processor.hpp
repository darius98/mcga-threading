#pragma once

#include <functional>
#include <tuple>

namespace mcga::threading::processors {

template<class... Objects>
class ObjectProcessor {
 public:
    using Object = std::tuple<Objects...>;

    explicit ObjectProcessor(std::function<void(Objects...)> func):
            func(func) {}

    void handleObject(const std::tuple<Objects...>& obj) {
        std::apply(func, obj);
    }

 private:
    std::function<void(Objects...)> func;
};

template<class T>
class ObjectProcessor<T> {
 public:
    using Object = T;

    explicit ObjectProcessor(std::function<void(T)> func):
            func(func) {}

    void handleObject(const Object& obj) {
        func(obj);
    }

 private:
    std::function<void(Object)> func;
};

}  // namespace mcga::threading::processors
