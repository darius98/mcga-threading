#pragma once

#include <functional>

namespace mcga::threading::processors {

template<class T>
class ObjectProcessor {
 public:
    using Object = T;

    explicit ObjectProcessor(std::function<void(const T&)> func): func(func) {}

    void handleObject(const T& obj) {
        func(obj);
    }

 private:
    std::function<void(const T&)> func;
};

}  // namespace mcga::threading::processors
