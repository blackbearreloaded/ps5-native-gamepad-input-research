#include "ps5_keyboard.hpp"
#include "ps5_mouse.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

std::array<ps5::keyboard::Data, 4> keyboard_fixture{};
std::int32_t keyboard_count{};
std::array<ps5::mouse::Data, 4> mouse_fixture{};
std::int32_t mouse_count{};

}  // namespace

extern "C" std::int32_t sceKeyboardRead(
    const std::int32_t handle, ps5::keyboard::Data* data,
    const std::int32_t capacity)
{
    static_cast<void>(handle);
    assert(capacity >= keyboard_count);
    std::copy_n(keyboard_fixture.begin(),
                static_cast<std::size_t>(keyboard_count), data);
    return keyboard_count;
}

extern "C" std::int32_t sceMouseRead(
    const std::int32_t handle, ps5::mouse::Data* data,
    const std::int32_t capacity)
{
    static_cast<void>(handle);
    assert(capacity >= mouse_count);
    std::copy_n(mouse_fixture.begin(), static_cast<std::size_t>(mouse_count),
                data);
    return mouse_count;
}

#include "../examples/08-keyboard-batch.cpp"
#include "../examples/09-mouse-batch.cpp"

int main()
{
    using namespace ps5::keyboard;

    examples::KeySet previous_keys{};
    struct KeyEdge {
        std::uint16_t usage;
        bool pressed;
    };
    std::array<KeyEdge, 4> key_edges{};
    std::size_t key_edge_count{};

    keyboard_count = 3;
    keyboard_fixture[0].connected = 1;
    keyboard_fixture[0].length = 1;
    keyboard_fixture[0].keycodes[0] = 0x04;
    keyboard_fixture[1] = keyboard_fixture[0];
    keyboard_fixture[1].keycodes[1] = 0x05;
    keyboard_fixture[1].length = 2;
    keyboard_fixture[2] = keyboard_fixture[1];
    keyboard_fixture[2].keycodes[0] = 0;
    keyboard_fixture[2].length = 1;

    const auto observe_key = [&key_edges, &key_edge_count](
                                 const Data&, const std::uint16_t usage,
                                 const bool pressed) noexcept {
        key_edges[key_edge_count++] = {usage, pressed};
    };
    assert(ps5::keyboard::examples::drain_keyboard_samples(
               1, previous_keys, observe_key) == 3);
    assert(key_edge_count == 3);
    assert(key_edges[0].usage == 0x04 && key_edges[0].pressed);
    assert(key_edges[1].usage == 0x05 && key_edges[1].pressed);
    assert(key_edges[2].usage == 0x04 && !key_edges[2].pressed);

    keyboard_count = 1;
    keyboard_fixture[0] = {};
    keyboard_fixture[0].length = 1;
    key_edge_count = 0;
    assert(ps5::keyboard::examples::drain_keyboard_samples(
               1, previous_keys, observe_key) == 1);
    assert(key_edge_count == 1);
    assert(key_edges[0].usage == 0x05 && !key_edges[0].pressed);

    using namespace ps5::mouse;
    ButtonMask previous_buttons = kButtonPrimary;
    std::size_t mouse_events{};

    mouse_count = 0;
    assert(ps5::mouse::examples::drain_mouse_samples(
               2, previous_buttons,
               [&mouse_events](const ps5::mouse::Data&, ButtonMask,
                               ButtonMask) noexcept { ++mouse_events; }) == 0);
    assert(previous_buttons == kButtonPrimary);
    assert(mouse_events == 0);

    mouse_count = 2;
    mouse_fixture[0].connected = 1;
    mouse_fixture[0].buttons = kButtonPrimary | kButtonX1;
    mouse_fixture[0].x = 7;
    mouse_fixture[1] = mouse_fixture[0];
    mouse_fixture[1].buttons = kButtonIntercepted;

    ButtonMask pressed{};
    ButtonMask released{};
    assert(ps5::mouse::examples::drain_mouse_samples(
               2, previous_buttons,
               [&mouse_events, &pressed, &released](
                   const ps5::mouse::Data&, const ButtonMask down,
                   const ButtonMask up) noexcept {
                   ++mouse_events;
                   pressed |= down;
                   released |= up;
               }) == 2);
    assert(mouse_events == 2);
    assert(pressed == kButtonX1);
    assert(released == (kButtonPrimary | kButtonX1));
    assert(previous_buttons == 0);
}
