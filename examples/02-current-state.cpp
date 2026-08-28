#include "ps5_pad.hpp"

#include <cstdint>

namespace ps5::pad::examples {

[[nodiscard]] std::int32_t read_current_state(
    const std::int32_t handle, Data& state) noexcept
{
    state = neutral_data();
    const auto result = scePadReadState(handle, &state);
    if (result < 0) {
        return result;
    }
    if (!is_usable(state)) {
        state = neutral_data(state.timestamp_us);
    }
    return 0;
}

}  // namespace ps5::pad::examples
