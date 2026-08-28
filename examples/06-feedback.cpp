#include "ps5_pad.hpp"

#include <cstdint>

namespace ps5::pad::examples {

[[nodiscard]] std::int32_t set_basic_feedback(
    const std::int32_t handle, const std::uint8_t low_frequency,
    const std::uint8_t high_frequency, const std::uint8_t red,
    const std::uint8_t green, const std::uint8_t blue) noexcept
{
    const Vibration vibration{
        .large_motor = low_frequency,
        .small_motor = high_frequency,
    };
    const Color color{.r = red, .g = green, .b = blue, .reserved = 0};
    const auto result = scePadSetVibration(handle, &vibration);
    return result < 0 ? result : scePadSetLightBar(handle, &color);
}

[[nodiscard]] std::int32_t set_trigger_feedback(
    const std::int32_t handle, const bool left, const bool right,
    const std::uint8_t position, const std::uint8_t strength) noexcept
{
    if (position > 9 || strength > 8) {
        return -1;
    }

    TriggerEffect effect{};
    if (left) {
        effect.trigger_mask |= kTriggerMaskL2;
        effect.l2.mode = static_cast<std::int32_t>(TriggerEffectMode::feedback);
        effect.l2.data[0] = position;
        effect.l2.data[1] = strength;
    }
    if (right) {
        effect.trigger_mask |= kTriggerMaskR2;
        effect.r2.mode = static_cast<std::int32_t>(TriggerEffectMode::feedback);
        effect.r2.data[0] = position;
        effect.r2.data[1] = strength;
    }
    return scePadSetTriggerEffect(handle, &effect);
}

void stop_feedback(const std::int32_t handle) noexcept
{
    const Vibration stop{};
    TriggerEffect triggers{};
    triggers.trigger_mask = kTriggerMaskL2 | kTriggerMaskR2;
    static_cast<void>(scePadSetVibration(handle, &stop));
    static_cast<void>(scePadSetTriggerEffect(handle, &triggers));
    static_cast<void>(scePadResetLightBar(handle));
}

}  // namespace ps5::pad::examples
