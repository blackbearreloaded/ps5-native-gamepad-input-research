#include "ps5_pad.hpp"
#include "ps5_keyboard.hpp"
#include "ps5_mouse.hpp"

#include <cassert>
#include <cstddef>

int main()
{
    using ps5::pad::ControllerInformation;
    using ps5::pad::Data;
    using ps5::pad::TriggerEffect;

    assert(sizeof(Data) == 120);
    assert(offsetof(Data, buttons) == 0x00);
    assert(offsetof(Data, left_stick) == 0x04);
    assert(offsetof(Data, triggers) == 0x08);
    assert(offsetof(Data, orientation) == 0x0c);
    assert(offsetof(Data, acceleration) == 0x1c);
    assert(offsetof(Data, angular_velocity) == 0x28);
    assert(offsetof(Data, touch) == 0x34);
    assert(offsetof(Data, connected) == 0x4c);
    assert(offsetof(Data, timestamp_us) == 0x50);
    assert(offsetof(Data, extension) == 0x58);
    assert(offsetof(Data, connected_count) == 0x68);
    assert(offsetof(Data, device_unique_data_length) == 0x6b);
    assert(offsetof(Data, device_unique_data) == 0x6c);
    assert(sizeof(ControllerInformation) == 28);
    assert(sizeof(TriggerEffect) == 120);

    using KeyboardData = ps5::keyboard::Data;
    using ps5::keyboard::OpenParameters;
    assert(sizeof(OpenParameters) == 8);
    assert(sizeof(KeyboardData) == 96);
    assert(offsetof(KeyboardData, timestamp_us) == 0x00);
    assert(offsetof(KeyboardData, intercepted) == 0x08);
    assert(offsetof(KeyboardData, connected) == 0x10);
    assert(offsetof(KeyboardData, length) == 0x14);
    assert(offsetof(KeyboardData, leds) == 0x18);
    assert(offsetof(KeyboardData, modifiers) == 0x1c);
    assert(offsetof(KeyboardData, keycodes) == 0x20);

    using MouseData = ps5::mouse::Data;
    using MouseOpenParameters = ps5::mouse::OpenParameters;
    assert(sizeof(MouseOpenParameters) == 8);
    assert(sizeof(MouseData) == 40);
    assert(offsetof(MouseData, timestamp_us) == 0x00);
    assert(offsetof(MouseData, connected) == 0x08);
    assert(offsetof(MouseData, buttons) == 0x0c);
    assert(offsetof(MouseData, x) == 0x10);
    assert(offsetof(MouseData, y) == 0x14);
    assert(offsetof(MouseData, wheel) == 0x18);
    assert(offsetof(MouseData, tilt) == 0x1c);
}
