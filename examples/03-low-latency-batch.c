#include "ps5_pad.h"

#include <string.h>

typedef void (*pad_sample_callback_t)(const ps5_pad_data_t *sample,
                                      uint32_t pressed,
                                      uint32_t released,
                                      void *context);

int drain_pad_samples(int32_t handle, uint32_t *previous_buttons,
                      pad_sample_callback_t callback, void *context)
{
    ps5_pad_data_t samples[PS5_PAD_MAX_SAMPLES];
    int32_t count = scePadRead(handle, samples, PS5_PAD_MAX_SAMPLES);

    if (count < 0)
        return count;
    for (int32_t i = 0; i < count; ++i) {
        const ps5_pad_data_t *sample = &samples[i];
        ps5_pad_data_t clean = *sample;
        uint32_t current = clean.buttons;
        if (!clean.connected ||
            (current & PS5_PAD_BUTTON_INTERCEPTED) != 0)
        {
            const uint64_t timestamp = clean.timestamp_us;
            memset(&clean, 0, sizeof(clean));
            clean.left_stick.x = clean.left_stick.y = 128;
            clean.right_stick.x = clean.right_stick.y = 128;
            clean.orientation.w = 1.0f;
            clean.timestamp_us = timestamp;
            current = 0;
        }

        const uint32_t changed = *previous_buttons ^ current;
        callback(&clean, changed & current,
                 changed & *previous_buttons, context);
        *previous_buttons = current;
    }
    return count;
}
