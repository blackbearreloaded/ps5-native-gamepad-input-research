#include "ps5_pad.h"

#include <limits.h>

typedef struct normalized_gamepad {
    uint32_t buttons;
    uint8_t left_trigger;
    uint8_t right_trigger;
    int16_t left_x;
    int16_t left_y;
    int16_t right_x;
    int16_t right_y;
} normalized_gamepad_t;

static int16_t normalize_axis(uint8_t value, bool invert)
{
    int32_t axis = ((int32_t)value - 128) * 256;
    if (invert)
        axis = -axis;
    if (axis > INT16_MAX)
        axis = INT16_MAX;
    if (axis < INT16_MIN)
        axis = INT16_MIN;
    return (int16_t)axis;
}

normalized_gamepad_t normalize_gamepad(const ps5_pad_data_t *sample)
{
    normalized_gamepad_t result = {0};
    if (!sample->connected ||
        (sample->buttons & PS5_PAD_BUTTON_INTERCEPTED) != 0)
        return result;

    result.buttons = sample->buttons;
    result.left_trigger = sample->triggers.l2;
    result.right_trigger = sample->triggers.r2;
    result.left_x = normalize_axis(sample->left_stick.x, false);
    result.left_y = normalize_axis(sample->left_stick.y, true);
    result.right_x = normalize_axis(sample->right_stick.x, false);
    result.right_y = normalize_axis(sample->right_stick.y, true);
    return result;
}
