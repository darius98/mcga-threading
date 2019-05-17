#pragma once

namespace mcga::threading::processors {

class StatelessFunctionProcessor {
  public:
    using Task = void (*)();

    static void executeTask(const Task& task) {
        task();
    }
};

}  // namespace mcga::threading::processors
