// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ps5::mouse {

inline constexpr std::int32_t kTypeStandard = 0;
inline constexpr std::size_t kSampleSize = 40;
// Batch capacity used by the device-tested application.
inline constexpr std::size_t kBatchSamples = 64;

using ButtonMask = std::uint32_t;

inline constexpr ButtonMask kButtonPrimary = 1U << 0U;
inline constexpr ButtonMask kButtonSecondary = 1U << 1U;

// The device-tested application passed eight zero bytes.
struct OpenParameters {
    std::array<std::uint8_t, 8> opaque;
};

struct Data {
    std::uint64_t timestamp_us;
    std::uint8_t connected;
    std::array<std::uint8_t, 3> padding0;
    ButtonMask buttons;
    std::int32_t x;
    std::int32_t y;
    std::int32_t wheel;
    std::int32_t wheel_horizontal;
    // Bytes this repository has not verified on a device.
    std::array<std::uint8_t, 8> unverified;
};

[[nodiscard]] constexpr bool is_usable(const Data& sample) noexcept
{
    return sample.connected != 0;
}

static_assert(sizeof(OpenParameters) == 8);
static_assert(sizeof(Data) == kSampleSize);
static_assert(offsetof(Data, connected) == 0x08);
static_assert(offsetof(Data, buttons) == 0x0c);
static_assert(offsetof(Data, x) == 0x10);
static_assert(offsetof(Data, y) == 0x14);
static_assert(offsetof(Data, wheel) == 0x18);
static_assert(offsetof(Data, wheel_horizontal) == 0x1c);
static_assert(std::is_standard_layout_v<Data>);
static_assert(std::is_trivially_copyable_v<Data>);

}  // namespace ps5::mouse

extern "C" {

std::int32_t sceMouseInit();
std::int32_t sceMouseOpen(
    std::int32_t user_id, std::int32_t type, std::int32_t index,
    const ps5::mouse::OpenParameters* parameters);
std::int32_t sceMouseClose(std::int32_t handle);
std::int32_t sceMouseRead(
    std::int32_t handle, ps5::mouse::Data* data,
    std::int32_t capacity);

}  // extern "C"
