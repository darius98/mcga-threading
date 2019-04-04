#pragma once

#include <functional>

namespace mcga::threading::processors {

class FunctionProcessor {
 public:
    using Object = std::function<void()>;

    static void handleObject(const Object& obj) {
        obj();
    }
};

}  // namespace mcga::threading::processors
