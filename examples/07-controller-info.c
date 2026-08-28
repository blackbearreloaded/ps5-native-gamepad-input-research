#include "ps5_pad.h"

typedef struct controller_capabilities {
    bool connected;
    int32_t device_class;
    uint8_t left_stick_dead_zone;
    uint8_t right_stick_dead_zone;
    uint16_t touch_width;
    uint16_t touch_height;
    uint8_t connection_generation;
} controller_capabilities_t;

int read_controller_capabilities(int32_t handle,
                                 controller_capabilities_t *capabilities)
{
    ps5_pad_controller_information_t information = {0};
    int32_t result = scePadGetControllerInformation(handle, &information);
    if (result < 0)
        return result;

    *capabilities = (controller_capabilities_t) {
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
