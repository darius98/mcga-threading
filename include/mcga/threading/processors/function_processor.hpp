#pragma once

#include <functional>

namespace mcga::threading::processors {

class FunctionProcessor {
 public:
    using Task = std::function<void()>;

    static void executeTask(const Task& task) {
        task();
    }
};

}  // namespace mcga::threading::processors
