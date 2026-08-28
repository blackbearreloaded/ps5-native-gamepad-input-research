#include "ps5_pad.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace ps5::pad::examples {

struct NormalizedGamepad {
    ButtonMask buttons{};
    std::uint8_t left_trigger{};
    std::uint8_t right_trigger{};
    std::int16_t left_x{};
    std::int16_t left_y{};
    std::int16_t right_x{};
    std::int16_t right_y{};
};

[[nodiscard]] constexpr std::int16_t normalize_axis(
    const std::uint8_t value, const bool invert) noexcept
{
    auto axis = (static_cast<std::int32_t>(value) - 128) * 256;
    if (invert) {
        axis = -axis;
    }
    axis = std::clamp(
        axis,
        static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::min()),
        static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::max()));
    return static_cast<std::int16_t>(axis);
}

[[nodiscard]] constexpr NormalizedGamepad normalize_gamepad(
    const Data& sample) noexcept
{
    if (!is_usable(sample)) {
        return {};
    }

    return {
        .buttons = sample.buttons,
        .left_trigger = sample.triggers.l2,
        .right_trigger = sample.triggers.r2,
        .left_x = normalize_axis(sample.left_stick.x, false),
        .left_y = normalize_axis(sample.left_stick.y, true),
        .right_x = normalize_axis(sample.right_stick.x, false),
        .right_y = normalize_axis(sample.right_stick.y, true),
    };
}

}  // namespace ps5::pad::examples
