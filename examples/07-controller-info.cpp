#include "ps5_pad.hpp"

#include <cstdint>

namespace ps5::pad::examples {

struct ControllerCapabilities {
    bool connected{};
    std::int32_t device_class{};
    std::uint8_t left_stick_dead_zone{};
    std::uint8_t right_stick_dead_zone{};
    std::uint16_t touch_width{};
    std::uint16_t touch_height{};
    std::uint8_t connection_generation{};
};

[[nodiscard]] std::int32_t read_controller_capabilities(
    const std::int32_t handle, ControllerCapabilities& capabilities) noexcept
{
    ControllerInformation information{};
    const auto result = scePadGetControllerInformation(handle, &information);
    if (result < 0) {
        return result;
    }

    capabilities = {
        .connected = information.connected != 0,
        .device_class = information.device_class,
        .left_stick_dead_zone = information.sticks.dead_zone_left,
        .right_stick_dead_zone = information.sticks.dead_zone_right,
        .touch_width = information.touchpad.resolution_x,
        .touch_height = information.touchpad.resolution_y,
        .connection_generation = information.connected_count,
    };
    return 0;
}

}  // namespace ps5::pad::examples
