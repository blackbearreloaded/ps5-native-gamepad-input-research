#include "ps5_pad.h"

#include <assert.h>

int main(void)
{
    assert(sizeof(ps5_pad_data_t) == 120);
    assert(offsetof(ps5_pad_data_t, buttons) == 0x00);
    assert(offsetof(ps5_pad_data_t, left_stick) == 0x04);
    assert(offsetof(ps5_pad_data_t, triggers) == 0x08);
    assert(offsetof(ps5_pad_data_t, orientation) == 0x0c);
    assert(offsetof(ps5_pad_data_t, acceleration) == 0x1c);
    assert(offsetof(ps5_pad_data_t, angular_velocity) == 0x28);
    assert(offsetof(ps5_pad_data_t, touch) == 0x34);
    assert(offsetof(ps5_pad_data_t, connected) == 0x4c);
    assert(offsetof(ps5_pad_data_t, timestamp_us) == 0x50);
    assert(offsetof(ps5_pad_data_t, extension) == 0x58);
    assert(offsetof(ps5_pad_data_t, connected_count) == 0x68);
    assert(offsetof(ps5_pad_data_t, device_unique_data_length) == 0x6b);
    assert(offsetof(ps5_pad_data_t, device_unique_data) == 0x6c);
    assert(sizeof(ps5_pad_controller_information_t) == 28);
    assert(sizeof(ps5_pad_trigger_effect_t) == 120);
    return 0;
}
