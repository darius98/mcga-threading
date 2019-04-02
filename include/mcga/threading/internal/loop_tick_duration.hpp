#pragma once

#include <chrono>

namespace mcga::threading::internal {

constexpr auto loopTickDuration = std::chrono::nanoseconds(20);

} // namespace mcga::threading::internal
