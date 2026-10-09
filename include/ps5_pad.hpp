// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ps5::pad {

inline constexpr std::int32_t kPortStandard = 0;
inline constexpr std::size_t kSampleSize = 120;
// Batch capacity used by the device-tested applications.
inline constexpr std::size_t kBatchSamples = 64;

using ButtonMask = std::uint32_t;

inline constexpr ButtonMask kButtonCreate = 0x00000001U;
inline constexpr ButtonMask kButtonL3 = 0x00000002U;
inline constexpr ButtonMask kButtonR3 = 0x00000004U;
inline constexpr ButtonMask kButtonOptions = 0x00000008U;
inline constexpr ButtonMask kButtonUp = 0x00000010U;
inline constexpr ButtonMask kButtonRight = 0x00000020U;
inline constexpr ButtonMask kButtonDown = 0x00000040U;
inline constexpr ButtonMask kButtonLeft = 0x00000080U;
inline constexpr ButtonMask kButtonL2 = 0x00000100U;
inline constexpr ButtonMask kButtonR2 = 0x00000200U;
inline constexpr ButtonMask kButtonL1 = 0x00000400U;
inline constexpr ButtonMask kButtonR1 = 0x00000800U;
inline constexpr ButtonMask kButtonTriangle = 0x00001000U;
inline constexpr ButtonMask kButtonCircle = 0x00002000U;
inline constexpr ButtonMask kButtonCross = 0x00004000U;
inline constexpr ButtonMask kButtonSquare = 0x00008000U;
inline constexpr ButtonMask kButtonTouchpadClick = 0x00100000U;
inline constexpr ButtonMask kButtonSystemOwned = 0x80000000U;

struct Stick {
    std::uint8_t x;
    std::uint8_t y;
};

struct TriggerLevels {
    std::uint8_t l2;
    std::uint8_t r2;
    std::array<std::uint8_t, 2> padding;
};

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Touch {
    std::uint16_t x;
    std::uint16_t y;
    std::uint8_t id;
    std::array<std::uint8_t, 3> padding;
};

struct TouchData {
    std::uint8_t count;
    std::array<std::uint8_t, 7> padding;
    std::array<Touch, 2> points;
};

struct Data {
    ButtonMask buttons;
    Stick left_stick;
    Stick right_stick;
    TriggerLevels triggers;
    Quaternion orientation;
    Vector3 acceleration;
    Vector3 angular_velocity;
    TouchData touch;
    std::int32_t connected;
    std::uint64_t timestamp_us;
    // Bytes this repository does not interpret.
    std::array<std::uint8_t, 16> opaque0;
    // Changes when a different controller attaches to the handle.
    std::uint8_t connection_generation;
    std::array<std::uint8_t, 15> opaque1;
};

[[nodiscard]] constexpr Data neutral_data(
    const std::uint64_t timestamp_us = 0) noexcept
{
    Data result{};
    result.left_stick = {128, 128};
    result.right_stick = {128, 128};
    result.orientation.w = 1.0F;
    result.timestamp_us = timestamp_us;
    return result;
}

[[nodiscard]] constexpr bool is_usable(const Data& sample) noexcept
{
    return sample.connected != 0 &&
           (sample.buttons & kButtonSystemOwned) == 0;
}

static_assert(sizeof(Data) == kSampleSize);
static_assert(offsetof(Data, touch) == 0x34);
static_assert(offsetof(Data, connected) == 0x4c);
static_assert(offsetof(Data, timestamp_us) == 0x50);
static_assert(offsetof(Data, connection_generation) == 0x68);
static_assert(std::is_standard_layout_v<Data>);
static_assert(std::is_trivially_copyable_v<Data>);

}  // namespace ps5::pad

extern "C" {

std::int32_t sceUserServiceInitialize(void* parameters);
std::int32_t sceUserServiceGetInitialUser(std::int32_t* user_id);
std::int32_t sceUserServiceTerminate();

std::int32_t scePadInit();
std::int32_t scePadOpen(std::int32_t user_id, std::int32_t port_type,
                        std::int32_t index, const void* parameters);
std::int32_t scePadClose(std::int32_t handle);
std::int32_t scePadReadState(std::int32_t handle, ps5::pad::Data* data);
std::int32_t scePadRead(std::int32_t handle, ps5::pad::Data* data,
                        std::int32_t capacity);

}  // extern "C"
