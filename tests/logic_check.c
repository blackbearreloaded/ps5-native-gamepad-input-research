#include "ps5_pad.h"

#include <assert.h>
#include <string.h>

static ps5_pad_data_t fixture[4];
static int32_t fixture_count;

int32_t scePadRead(int32_t handle, ps5_pad_data_t *data, int32_t capacity)
{
    (void)handle;
    assert(capacity >= fixture_count);
    memcpy(data, fixture, (size_t)fixture_count * sizeof(*data));
    return fixture_count;
}

#include "../examples/03-low-latency-batch.c"
#include "../examples/04-full-joystick.c"

typedef struct observed_edges {
    uint32_t pressed[4];
    uint32_t released[4];
    ps5_pad_data_t samples[4];
    int count;
} observed_edges_t;

static void observe(const ps5_pad_data_t *sample, uint32_t pressed,
                    uint32_t released, void *context)
{
    observed_edges_t *observed = context;
    const int index = observed->count++;
    observed->pressed[index] = pressed;
    observed->released[index] = released;
    observed->samples[index] = *sample;
}

int main(void)
{
    uint32_t previous = 0;
    observed_edges_t observed = {0};

    fixture_count = 3;
    fixture[0].connected = 1;
    fixture[0].left_stick.x = fixture[0].left_stick.y = 128;
    fixture[0].right_stick.x = fixture[0].right_stick.y = 128;
    fixture[0].buttons = PS5_PAD_BUTTON_CROSS;
    fixture[1] = fixture[0];
    fixture[1].buttons = 0;
    fixture[2] = fixture[0];
    fixture[2].buttons = PS5_PAD_BUTTON_INTERCEPTED;
    fixture[2].left_stick.x = 255;
    fixture[2].timestamp_us = 1234;

    assert(drain_pad_samples(1, &previous, observe, &observed) == 3);
    assert(observed.count == 3);
    assert(observed.pressed[0] == PS5_PAD_BUTTON_CROSS);
    assert(observed.released[1] == PS5_PAD_BUTTON_CROSS);
    assert(observed.samples[2].buttons == 0);
    assert(observed.samples[2].left_stick.x == 128);
    assert(observed.samples[2].timestamp_us == 1234);

    ps5_pad_data_t raw = {0};
    raw.connected = 1;
    raw.left_stick.x = 0;
    raw.left_stick.y = 0;
    raw.right_stick.x = 255;
    raw.right_stick.y = 255;
    raw.triggers.l2 = 17;
    raw.triggers.r2 = 231;
    normalized_gamepad_t gamepad = normalize_gamepad(&raw);
    assert(gamepad.left_x == INT16_MIN);
    assert(gamepad.left_y == INT16_MAX);
    assert(gamepad.right_x == 32512);
    assert(gamepad.right_y == -32512);
    assert(gamepad.left_trigger == 17);
    assert(gamepad.right_trigger == 231);
    return 0;
}
