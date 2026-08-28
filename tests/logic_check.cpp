#include "ps5_pad.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

std::array<ps5::pad::Data, 4> fixture{};
std::int32_t fixture_count{};

}  // namespace

extern "C" std::int32_t scePadRead(
    const std::int32_t handle, ps5::pad::Data* data,
    const std::int32_t capacity)
{
    static_cast<void>(handle);
    assert(capacity >= fixture_count);
    std::copy_n(fixture.begin(), static_cast<std::size_t>(fixture_count), data);
    return fixture_count;
}

#include "../examples/03-low-latency-batch.cpp"
#include "../examples/04-full-joystick.cpp"

namespace {

struct ObservedEdges {
    std::array<ps5::pad::ButtonMask, 4> pressed{};
    std::array<ps5::pad::ButtonMask, 4> released{};
    std::array<ps5::pad::Data, 4> samples{};
    std::size_t count{};
};

}  // namespace

int main()
{
    using namespace ps5::pad;
    using namespace ps5::pad::examples;

    ButtonMask previous{};
    ObservedEdges observed{};

    fixture_count = 3;
    fixture[0] = neutral_data();
    fixture[0].connected = 1;
    fixture[0].buttons = kButtonCross;
    fixture[1] = fixture[0];
    fixture[1].buttons = 0;
    fixture[2] = fixture[0];
    fixture[2].buttons = kButtonIntercepted;
    fixture[2].left_stick.x = 255;
    fixture[2].timestamp_us = 1234;

    const auto observe = [&observed](
                             const Data& sample, const ButtonMask pressed,
                             const ButtonMask released) noexcept {
        const auto index = observed.count++;
        observed.pressed[index] = pressed;
        observed.released[index] = released;
        observed.samples[index] = sample;
    };

    assert(drain_pad_samples(1, previous, observe) == 3);
    assert(observed.count == 3);
    assert(observed.pressed[0] == kButtonCross);
    assert(observed.released[1] == kButtonCross);
    assert(observed.samples[2].buttons == 0);
    assert(observed.samples[2].left_stick.x == 128);
    assert(observed.samples[2].timestamp_us == 1234);

    Data raw{};
    raw.connected = 1;
    raw.left_stick = {0, 0};
    raw.right_stick = {255, 255};
    raw.triggers.l2 = 17;
    raw.triggers.r2 = 231;
    const auto gamepad = normalize_gamepad(raw);
    assert(gamepad.left_x == std::numeric_limits<std::int16_t>::min());
    assert(gamepad.left_y == std::numeric_limits<std::int16_t>::max());
    assert(gamepad.right_x == 32512);
    assert(gamepad.right_y == -32512);
    assert(gamepad.left_trigger == 17);
    assert(gamepad.right_trigger == 231);
}
