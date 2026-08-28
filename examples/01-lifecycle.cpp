#include "ps5_pad.hpp"

#include <cstdint>

namespace ps5::pad::examples {

class Session final {
public:
    Session() noexcept = default;
    ~Session() noexcept { close(); }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    [[nodiscard]] std::int32_t open() noexcept
    {
        close();

        const auto user_service_result = sceUserServiceInitialize(nullptr);
        owns_user_service_ = user_service_result == 0;

        std::int32_t user_id = -1;
        auto result = sceUserServiceGetInitialUser(&user_id);
        if (result < 0) {
            close();
            return result;
        }

        result = scePadInit();
        if (result < 0) {
            close();
            return result;
        }

        handle_ = scePadOpen(user_id, kPortTypeStandard, 0, nullptr);
        if (handle_ < 0) {
            result = handle_;
            close();
            return result;
        }
        return 0;
    }

    void close() noexcept
    {
        if (handle_ >= 0) {
            const Vibration stop{};
            static_cast<void>(scePadSetVibration(handle_, &stop));
            static_cast<void>(scePadResetLightBar(handle_));
            static_cast<void>(scePadClose(handle_));
            handle_ = -1;
        }
        if (owns_user_service_) {
            static_cast<void>(sceUserServiceTerminate());
            owns_user_service_ = false;
        }
    }

    [[nodiscard]] std::int32_t handle() const noexcept { return handle_; }

private:
    std::int32_t handle_ = -1;
    bool owns_user_service_ = false;
};

}  // namespace ps5::pad::examples
