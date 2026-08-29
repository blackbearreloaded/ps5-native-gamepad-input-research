#include "ps5_keyboard.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

namespace ps5::keyboard::examples {

using KeySet = std::array<std::uint16_t, kMaxKeys>;

[[nodiscard]] constexpr bool contains(
    const KeySet& keys, const std::uint16_t usage) noexcept
{
    return usage != 0 &&
           std::find(keys.begin(), keys.end(), usage) != keys.end();
}

[[nodiscard]] constexpr KeySet active_keys(const Data& sample) noexcept
{
    KeySet result{};
    if (!is_usable(sample)) {
        return result;
    }

    std::size_t output = 0;
    for (const auto usage : sample.keycodes) {
        if (usage != 0 && !contains(result, usage)) {
            result[output++] = usage;
        }
    }
    return result;
}

template <typename Callback>
[[nodiscard]] std::int32_t drain_keyboard_samples(
    const std::int32_t handle, KeySet& previous, Callback&& callback)
{
    std::array<Data, kMaxSamples> samples{};
    const auto count = sceKeyboardRead(
        handle, samples.data(), static_cast<std::int32_t>(samples.size()));
    if (count < 0) {
        return count;
    }

    const auto returned =
        std::min(static_cast<std::size_t>(count), samples.size());
    for (const auto& sample :
         std::span<const Data>{samples}.first(returned)) {
        const auto current = active_keys(sample);
        for (const auto usage : previous) {
            if (usage != 0 && !contains(current, usage)) {
                std::invoke(callback, std::as_const(sample), usage, false);
            }
        }
        for (const auto usage : current) {
            if (usage != 0 && !contains(previous, usage)) {
                std::invoke(callback, std::as_const(sample), usage, true);
            }
        }
        previous = current;
    }
    return count;
}

}  // namespace ps5::keyboard::examples
