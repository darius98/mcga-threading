#pragma once

#include <functional>
#include <tuple>

namespace mcga::threading::processors {

template<class... Args>
class ObjectProcessor {
 public:
    using Task = std::tuple<Args...>;

    explicit ObjectProcessor(std::function<void(Args...)> func):
            func(func) {}

    void executeTask(const Task& task) {
        std::apply(func, task);
    }

 private:
    std::function<void(Args...)> func;
};

template<class T>
class ObjectProcessor<T> {
 public:
    using Task = T;

    explicit ObjectProcessor(std::function<void(const T&)> func):
            func(func) {}

    void executeTask(const Task& task) {
        func(task);
    }

 private:
    std::function<void(const T&)> func;
};

}  // namespace mcga::threading::processors
