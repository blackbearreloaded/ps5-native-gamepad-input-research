#include "ps5_pad.h"

#include <string.h>

int set_basic_feedback(int32_t handle, uint8_t low_frequency,
                       uint8_t high_frequency,
                       uint8_t red, uint8_t green, uint8_t blue)
{
    const ps5_pad_vibration_t vibration = {
        .large_motor = low_frequency,
        .small_motor = high_frequency,
    };
    const ps5_pad_color_t color = {.r = red, .g = green, .b = blue};
    int32_t result = scePadSetVibration(handle, &vibration);
    return result < 0 ? result : scePadSetLightBar(handle, &color);
}

int set_trigger_feedback(int32_t handle, bool left, bool right,
                         uint8_t position, uint8_t strength)
{
    ps5_pad_trigger_effect_t effect;
    if (position > 9 || strength > 8)
        return -1;
    memset(&effect, 0, sizeof(effect));

    if (left) {
        effect.trigger_mask |= PS5_PAD_TRIGGER_MASK_L2;
        effect.l2.mode = PS5_PAD_TRIGGER_EFFECT_FEEDBACK;
        effect.l2.data[0] = position;
        effect.l2.data[1] = strength;
    }
    if (right) {
        effect.trigger_mask |= PS5_PAD_TRIGGER_MASK_R2;
        effect.r2.mode = PS5_PAD_TRIGGER_EFFECT_FEEDBACK;
        effect.r2.data[0] = position;
        effect.r2.data[1] = strength;
    }
    return scePadSetTriggerEffect(handle, &effect);
}

void stop_feedback(int32_t handle)
{
    const ps5_pad_vibration_t stop = {0};
    ps5_pad_trigger_effect_t triggers;
    memset(&triggers, 0, sizeof(triggers));
    triggers.trigger_mask = PS5_PAD_TRIGGER_MASK_L2 |
                            PS5_PAD_TRIGGER_MASK_R2;
    (void)scePadSetVibration(handle, &stop);
    (void)scePadSetTriggerEffect(handle, &triggers);
    (void)scePadResetLightBar(handle);
}
