// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ps5::keyboard {

inline constexpr std::int32_t kTypeStandard = 0;
inline constexpr std::size_t kSampleSize = 96;
// Batch capacity used by the device-tested application.
inline constexpr std::size_t kBatchSamples = 16;
inline constexpr std::size_t kKeySlots = 16;

// The device-tested application passed eight zero bytes.
struct OpenParameters {
    std::array<std::uint8_t, 8> opaque;
};

struct Data {
    std::uint64_t timestamp_us;
    std::uint8_t system_owned;
    std::array<std::uint8_t, 7> padding0;
    std::uint8_t connected;
    std::array<std::uint8_t, 3> padding1;
    std::int32_t key_count;
    // Bytes this repository has not verified on a device.
    std::array<std::uint8_t, 8> unverified0;
    // USB HID keyboard usage values; zero means no key.
    std::array<std::uint16_t, kKeySlots> usages;
    std::array<std::uint8_t, 32> unverified1;
};

[[nodiscard]] constexpr bool is_usable(const Data& sample) noexcept
{
    return sample.connected != 0 && sample.system_owned == 0;
}

[[nodiscard]] constexpr bool contains_usage(
    const Data& sample, const std::uint16_t usage) noexcept
{
    if (usage == 0 || !is_usable(sample)) {
        return false;
    }
    for (const auto keycode : sample.usages) {
        if (keycode == usage) {
            return true;
        }
    }
    return false;
}

static_assert(sizeof(OpenParameters) == 8);
static_assert(sizeof(Data) == kSampleSize);
static_assert(offsetof(Data, system_owned) == 0x08);
static_assert(offsetof(Data, connected) == 0x10);
static_assert(offsetof(Data, key_count) == 0x14);
static_assert(offsetof(Data, usages) == 0x20);
static_assert(std::is_standard_layout_v<Data>);
static_assert(std::is_trivially_copyable_v<Data>);

}  // namespace ps5::keyboard

extern "C" {

std::int32_t sceKeyboardInit();
std::int32_t sceKeyboardOpen(
    std::int32_t user_id, std::int32_t type, std::int32_t index,
    const ps5::keyboard::OpenParameters* parameters);
std::int32_t sceKeyboardClose(std::int32_t handle);
std::int32_t sceKeyboardRead(
    std::int32_t handle, ps5::keyboard::Data* data,
    std::int32_t capacity);

}  // extern "C"
