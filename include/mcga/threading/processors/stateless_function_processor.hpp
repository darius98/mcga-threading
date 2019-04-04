#pragma once

namespace mcga::threading::processors {

class StatelessFunctionProcessor {
 public:
    using Object = void (*)();

    static void handleObject(const Object& obj) {
        obj();
    }
};

}  // namespace mcga::threading::processors
