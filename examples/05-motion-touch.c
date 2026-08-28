#include "ps5_pad.h"

int configure_motion(int32_t handle)
{
    int32_t result = scePadSetMotionSensorState(handle, true);
    if (result < 0)
        return result;
    result = scePadSetTiltCorrectionState(handle, true);
    if (result < 0)
        return result;
    result = scePadSetAngularVelocityDeadbandState(handle, true);
    if (result < 0)
        return result;
    return scePadResetOrientation(handle);
}

void consume_motion_and_touch(const ps5_pad_data_t *sample,
                              ps5_pad_quaternion_t *orientation,
                              ps5_pad_vector3_t *acceleration,
                              ps5_pad_vector3_t *angular_velocity,
                              ps5_pad_touch_t touches[2],
                              uint8_t *touch_count)
{
    *orientation = sample->orientation;
    *acceleration = sample->acceleration;
    *angular_velocity = sample->angular_velocity;
    *touch_count = sample->touch.count > 2 ? 2 : sample->touch.count;
    for (uint8_t i = 0; i < *touch_count; ++i)
        touches[i] = sample->touch.points[i];
}
