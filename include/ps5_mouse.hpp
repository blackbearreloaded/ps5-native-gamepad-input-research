#pragma once

/*
 * Independently authored interoperability declarations.
 * No vendor SDK header or source code is included; see ../NOTICE.md.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ps5::mouse {

inline constexpr std::int32_t kTypeStandard = 0;
inline constexpr std::size_t kSampleSize = 40;
inline constexpr std::size_t kMaxSamples = 64;
inline constexpr std::uint8_t kBehaviorMerged = 1U << 0U;

using ButtonMask = std::uint32_t;

inline constexpr ButtonMask kButtonPrimary = 1U << 0U;
inline constexpr ButtonMask kButtonSecondary = 1U << 1U;
inline constexpr ButtonMask kButtonMiddle = 1U << 2U;
inline constexpr ButtonMask kButtonX1 = 1U << 3U;
inline constexpr ButtonMask kButtonX2 = 1U << 4U;
inline constexpr ButtonMask kButtonIntercepted = 1U << 31U;

struct OpenParameters {
    std::uint8_t behavior;
    std::array<std::uint8_t, 7> reserved;
};

struct Data {
    std::uint64_t timestamp_us;
    std::uint8_t connected;
    std::array<std::uint8_t, 3> reserved0;
    ButtonMask buttons;
    std::int32_t x;
    std::int32_t y;
    std::int32_t wheel;
    std::int32_t tilt;
    std::array<std::uint8_t, 8> reserved1;
};

[[nodiscard]] constexpr bool is_usable(const Data& sample) noexcept
{
    return sample.connected != 0 &&
           (sample.buttons & kButtonIntercepted) == 0;
}

static_assert(sizeof(OpenParameters) == 8);
static_assert(sizeof(Data) == kSampleSize);
static_assert(offsetof(Data, connected) == 0x08);
static_assert(offsetof(Data, buttons) == 0x0c);
static_assert(offsetof(Data, x) == 0x10);
static_assert(offsetof(Data, y) == 0x14);
static_assert(offsetof(Data, wheel) == 0x18);
static_assert(offsetof(Data, tilt) == 0x1c);
static_assert(std::is_standard_layout_v<Data>);
static_assert(std::is_trivially_copyable_v<Data>);

}  // namespace ps5::mouse

extern "C" {

std::int32_t sceMouseInit();
std::int32_t sceMouseOpen(
    std::int32_t user_id, std::int32_t type, std::int32_t index,
    const ps5::mouse::OpenParameters* parameters);
std::int32_t sceMouseClose(std::int32_t handle);
std::int32_t sceMouseGetHandle(
    std::int32_t user_id, std::int32_t type, std::int32_t index);
std::int32_t sceMouseRead(
    std::int32_t handle, ps5::mouse::Data* data,
    std::int32_t capacity);
std::int32_t sceMouseSetHandType(std::int32_t handle, std::int32_t type);
std::int32_t sceMouseSetPointerSpeed(
    std::int32_t handle, std::int32_t speed);

}  // extern "C"
