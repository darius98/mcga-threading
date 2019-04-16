#pragma once

#include <chrono>

namespace mcga::threading::base {

constexpr auto loopTickDuration = std::chrono::nanoseconds(20);

}  // namespace mcga::threading::base
