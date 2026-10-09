// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_pad.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace ps5::pad::examples {

struct MotionTouchSnapshot {
    Quaternion orientation{};
    Vector3 acceleration{};
    Vector3 angular_velocity{};
    std::array<Touch, 2> touches{};
    std::size_t touch_count{};
};

[[nodiscard]] MotionTouchSnapshot capture_motion_and_touch(
    const Data& sample) noexcept
{
    MotionTouchSnapshot result{
        .orientation = sample.orientation,
        .acceleration = sample.acceleration,
        .angular_velocity = sample.angular_velocity,
    };
    result.touch_count = std::min<std::size_t>(
        sample.touch.count, result.touches.size());
    std::copy_n(sample.touch.points.begin(), result.touch_count,
                result.touches.begin());
    return result;
}

}  // namespace ps5::pad::examples
