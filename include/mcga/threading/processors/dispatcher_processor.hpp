#pragma once

#include <functional>
#include <tuple>
#include <vector>

namespace mcga::threading::processors {

template<class... Args>
class DispatcherProcessor {
 public:
    using Task = std::tuple<Args...>;

    using Callback = std::function<void(Args...)>;

    std::vector<Callback> callbacks;

    void addCallback(Callback callback) {
        callbacks.push_back(std::move(callback));
    }

    void handleTask(const Task& task) {
        for (const Callback& callback: callbacks) {
            std::apply(callback, task);
        }
    }
};

template<class T>
class DispatcherProcessor<T> {
public:
    using Task = T;

    using Callback = std::function<void(T&&)>;

    std::vector<Callback> callbacks;

    void addCallback(Callback callback) {
        callbacks.push_back(std::move(callback));
    }

    void executeTask(Task&& task) {
        for (const Callback& callback: callbacks) {
            callback(std::move(task));
        }
    }
};

}  // namespace mcga::threading::processors
