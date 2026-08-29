#include "ps5_mouse.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

namespace ps5::mouse::examples {

template <typename Callback>
[[nodiscard]] std::int32_t drain_mouse_samples(
    const std::int32_t handle, ButtonMask& previous_buttons,
    Callback&& callback)
{
    std::array<Data, kMaxSamples> samples{};
    const auto count = sceMouseRead(
        handle, samples.data(), static_cast<std::int32_t>(samples.size()));
    if (count <= 0) {
        // Zero means no output record was returned. Do not parse the buffer
        // or clear the remembered held-button state.
        return count;
    }

    const auto returned =
        std::min(static_cast<std::size_t>(count), samples.size());
    for (const auto& sample :
         std::span<const Data>{samples}.first(returned)) {
        const auto clean = is_usable(sample) ? sample : Data{};
        const auto current = clean.buttons;
        const auto changed = previous_buttons ^ current;
        std::invoke(callback, std::as_const(clean), changed & current,
                    changed & previous_buttons);
        previous_buttons = current;
    }
    return count;
}

}  // namespace ps5::mouse::examples
