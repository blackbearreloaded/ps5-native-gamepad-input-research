#include "ps5_keyboard.hpp"
#include "ps5_mouse.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <string_view>
#include <unistd.h>

extern "C" {

std::int32_t sceUserServiceInitialize(void* parameters);
std::int32_t sceUserServiceGetInitialUser(std::int32_t* user_id);
std::int32_t sceUserServiceTerminate();
std::int32_t sceKernelSendNotificationRequest(
    std::uint32_t device, void* request, std::size_t size,
    std::int32_t blocking);
std::int32_t sceKernelUsleep(std::uint32_t microseconds);
std::int32_t sceSysmoduleLoadModule(std::uint16_t id);
std::int32_t sceSysmoduleUnloadModule(std::uint16_t id);

}

namespace {

constexpr std::string_view kLogPath = "/download0/ps5-input-probe.log";
constexpr std::uint16_t kKeyboardSysmodule = 0x0106;
constexpr std::uint16_t kMouseSysmodule = 0x00a9;

struct NotificationRequest {
    std::array<std::uint8_t, 45> reserved{};
    std::array<char, 3075> message{};
};

class Line final {
public:
    void append(const std::string_view value) noexcept
    {
        for (const char character : value) {
            if (size_ + 1 >= bytes_.size()) {
                break;
            }
            bytes_[size_++] = character;
        }
        bytes_[size_] = '\0';
    }

    void append_integer(const std::int64_t value) noexcept
    {
        auto magnitude = value;
        if (magnitude < 0) {
            append("-");
            magnitude = -magnitude;
        }

        std::array<char, 24> digits{};
        std::size_t count = 0;
        do {
            digits[count++] =
                static_cast<char>('0' + static_cast<char>(magnitude % 10));
            magnitude /= 10;
        } while (magnitude != 0 && count < digits.size());

        while (count != 0) {
            const char digit = digits[--count];
            append(std::string_view{&digit, 1});
        }
    }

    [[nodiscard]] const char* data() const noexcept { return bytes_.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    std::array<char, 256> bytes_{};
    std::size_t size_{};
};

NotificationRequest notification{};

void notify(const std::string_view message) noexcept
{
    const auto count =
        message.size() < notification.message.size() - 1
            ? message.size()
            : notification.message.size() - 1;
    for (std::size_t index = 0; index < count; ++index) {
        notification.message[index] = message[index];
    }
    notification.message[count] = '\0';
    static_cast<void>(sceKernelSendNotificationRequest(
        0, &notification, sizeof(notification), 0));
}

void reset_log() noexcept
{
    const int descriptor =
        open(kLogPath.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (descriptor >= 0) {
        static_cast<void>(close(descriptor));
    }
}

void record(Line& line) noexcept
{
    line.append("\n");
    const int descriptor =
        open(kLogPath.data(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (descriptor < 0) {
        return;
    }
    static_cast<void>(write(descriptor, line.data(), line.size()));
    static_cast<void>(close(descriptor));
}

void record_checkpoint(
    const std::string_view name, const std::int32_t result) noexcept
{
    Line line;
    line.append("CHECKPOINT ");
    line.append(name);
    line.append(" result=");
    line.append_integer(result);
    record(line);
}

class InputSession final {
public:
    InputSession() noexcept = default;
    ~InputSession() noexcept
    {
        if (mouse_handle >= 0) {
            static_cast<void>(sceMouseClose(mouse_handle));
        }
        if (keyboard_handle >= 0) {
            static_cast<void>(sceKeyboardClose(keyboard_handle));
        }
        if (mouse_module_result == 0) {
            static_cast<void>(sceSysmoduleUnloadModule(kMouseSysmodule));
        }
        if (keyboard_module_result == 0) {
            static_cast<void>(sceSysmoduleUnloadModule(kKeyboardSysmodule));
        }
        if (owns_user_service) {
            static_cast<void>(sceUserServiceTerminate());
        }
    }

    InputSession(const InputSession&) = delete;
    InputSession& operator=(const InputSession&) = delete;

    void open() noexcept
    {
        user_service_result = sceUserServiceInitialize(nullptr);
        record_checkpoint("user-service-init", user_service_result);
        owns_user_service = user_service_result == 0;
        user_result = sceUserServiceGetInitialUser(&user_id);
        record_checkpoint("initial-user", user_result);
        if (user_result < 0) {
            return;
        }

        keyboard_module_result = sceSysmoduleLoadModule(kKeyboardSysmodule);
        record_checkpoint("keyboard-sysmodule", keyboard_module_result);
        if (keyboard_module_result >= 0) {
            keyboard_init_result = sceKeyboardInit();
            record_checkpoint("keyboard-init", keyboard_init_result);
        }
        if (keyboard_init_result >= 0) {
            const ps5::keyboard::OpenParameters parameters{};
            keyboard_handle = sceKeyboardOpen(
                user_id, ps5::keyboard::kTypeStandard, 0, &parameters);
            record_checkpoint("keyboard-open", keyboard_handle);
        }

        mouse_module_result = sceSysmoduleLoadModule(kMouseSysmodule);
        record_checkpoint("mouse-sysmodule", mouse_module_result);
        if (mouse_module_result >= 0) {
            mouse_init_result = sceMouseInit();
            record_checkpoint("mouse-init", mouse_init_result);
        }
        if (mouse_init_result >= 0) {
            const ps5::mouse::OpenParameters parameters{};
            mouse_handle = sceMouseOpen(
                user_id, ps5::mouse::kTypeStandard, 0, &parameters);
            record_checkpoint("mouse-open", mouse_handle);
        }
    }

    std::int32_t user_service_result{-1};
    std::int32_t user_result{-1};
    std::int32_t keyboard_init_result{-1};
    std::int32_t keyboard_handle{-1};
    std::int32_t keyboard_module_result{-1};
    std::int32_t mouse_init_result{-1};
    std::int32_t mouse_handle{-1};
    std::int32_t mouse_module_result{-1};
    std::int32_t user_id{-1};
    bool owns_user_service{};
};

void record_start(const InputSession& session) noexcept
{
    Line line;
    line.append("START user=");
    line.append_integer(session.user_id);
    line.append(" userResult=");
    line.append_integer(session.user_result);
    line.append(" keyboardInit=");
    line.append_integer(session.keyboard_init_result);
    line.append(" keyboardModule=");
    line.append_integer(session.keyboard_module_result);
    line.append(" keyboardHandle=");
    line.append_integer(session.keyboard_handle);
    line.append(" mouseInit=");
    line.append_integer(session.mouse_init_result);
    line.append(" mouseModule=");
    line.append_integer(session.mouse_module_result);
    line.append(" mouseHandle=");
    line.append_integer(session.mouse_handle);
    record(line);
}

void record_keyboard(const ps5::keyboard::Data& sample) noexcept
{
    std::uint16_t first_usage = 0;
    for (const auto usage : sample.keycodes) {
        if (usage != 0) {
            first_usage = usage;
            break;
        }
    }

    Line line;
    line.append("KEYBOARD timestamp=");
    line.append_integer(static_cast<std::int64_t>(sample.timestamp_us));
    line.append(" connected=");
    line.append_integer(sample.connected);
    line.append(" intercepted=");
    line.append_integer(sample.intercepted);
    line.append(" length=");
    line.append_integer(sample.length);
    line.append(" modifiers=");
    line.append_integer(sample.modifiers);
    line.append(" firstUsage=");
    line.append_integer(first_usage);
    record(line);
}

[[nodiscard]] bool has_active_usage(
    const ps5::keyboard::Data& sample) noexcept
{
    return std::any_of(sample.keycodes.begin(), sample.keycodes.end(),
                       [](const std::uint16_t usage) { return usage != 0; });
}

void record_mouse(const ps5::mouse::Data& sample) noexcept
{
    Line line;
    line.append("MOUSE timestamp=");
    line.append_integer(static_cast<std::int64_t>(sample.timestamp_us));
    line.append(" connected=");
    line.append_integer(sample.connected);
    line.append(" buttons=");
    line.append_integer(sample.buttons);
    line.append(" x=");
    line.append_integer(sample.x);
    line.append(" y=");
    line.append_integer(sample.y);
    line.append(" wheel=");
    line.append_integer(sample.wheel);
    line.append(" tilt=");
    line.append_integer(sample.tilt);
    record(line);
}

}  // namespace

int main()
{
    notify("PS5 input probe: entered main");
    reset_log();
    record_checkpoint("entered-main", 0);

    InputSession session;
    session.open();
    record_start(session);

    if (session.keyboard_handle < 0 && session.mouse_handle < 0) {
        notify("PS5 input probe: keyboard and mouse open failed");
    } else {
        notify("PS5 input probe ready: type and move/click");
    }

    bool keyboard_observed = false;
    bool mouse_observed = false;
    bool keyboard_read_error = false;
    bool mouse_read_error = false;
    bool keyboard_state_recorded = false;
    std::size_t keyboard_record_count = 0;
    std::size_t mouse_record_count = 0;
    ps5::keyboard::Data previous_keyboard{};
    std::array<ps5::keyboard::Data, ps5::keyboard::kMaxSamples>
        keyboard_samples{};
    std::array<ps5::mouse::Data, ps5::mouse::kMaxSamples> mouse_samples{};

    for (;;) {
        if (session.keyboard_handle >= 0) {
            const auto count = sceKeyboardRead(
                session.keyboard_handle, keyboard_samples.data(),
                static_cast<std::int32_t>(keyboard_samples.size()));
            if (count > 0) {
                const auto returned = std::min(
                    static_cast<std::size_t>(count), keyboard_samples.size());
                for (const auto& sample :
                     std::span{keyboard_samples}.first(returned)) {
                    const bool active =
                        has_active_usage(sample) || sample.modifiers != 0;
                    const bool changed =
                        !keyboard_state_recorded ||
                        sample.connected != previous_keyboard.connected ||
                        sample.intercepted != previous_keyboard.intercepted ||
                        sample.leds != previous_keyboard.leds ||
                        sample.modifiers != previous_keyboard.modifiers ||
                        sample.keycodes != previous_keyboard.keycodes;
                    if (changed && keyboard_record_count < 64) {
                        record_keyboard(sample);
                        ++keyboard_record_count;
                    }
                    previous_keyboard = sample;
                    keyboard_state_recorded = true;
                    if (!keyboard_observed && ps5::keyboard::is_usable(sample) &&
                        active) {
                        keyboard_observed = true;
                        notify("PS5 input probe: keyboard input captured");
                    }
                }
            } else if (count < 0 && !keyboard_read_error) {
                keyboard_read_error = true;
                record_checkpoint("keyboard-read", count);
            }
        }

        if (session.mouse_handle >= 0) {
            const auto count = sceMouseRead(
                session.mouse_handle, mouse_samples.data(),
                static_cast<std::int32_t>(mouse_samples.size()));
            if (count > 0) {
                const auto returned = std::min(
                    static_cast<std::size_t>(count), mouse_samples.size());
                for (const auto& sample :
                     std::span{mouse_samples}.first(returned)) {
                    if (mouse_record_count < 128) {
                        record_mouse(sample);
                        ++mouse_record_count;
                    }
                    const bool changed = sample.x != 0 || sample.y != 0 ||
                                         sample.wheel != 0 || sample.tilt != 0 ||
                                         sample.buttons != 0;
                    if (!mouse_observed && ps5::mouse::is_usable(sample) &&
                        changed) {
                        mouse_observed = true;
                        notify("PS5 input probe: mouse input captured");
                    }
                }
            } else if (count < 0 && !mouse_read_error) {
                mouse_read_error = true;
                record_checkpoint("mouse-read", count);
            }
        }

        static_cast<void>(sceKernelUsleep(4000));
    }
}
