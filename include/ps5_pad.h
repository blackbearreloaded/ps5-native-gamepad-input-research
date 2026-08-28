#ifndef PS5_PAD_H
#define PS5_PAD_H

/*
 * Independently authored interoperability declarations.
 * No vendor SDK header or source code is included; see ../NOTICE.md.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PS5_PAD_PORT_TYPE_STANDARD = 0,
    PS5_PAD_PORT_TYPE_SPECIAL = 2,
    PS5_PAD_PORT_TYPE_REMOTE_CONTROL = 16,
    PS5_PAD_SAMPLE_SIZE = 120,
    PS5_PAD_MAX_SAMPLES = 64,
};

typedef uint32_t ps5_pad_button_t;

#define PS5_PAD_BUTTON_CREATE      UINT32_C(0x00000001)
#define PS5_PAD_BUTTON_L3          UINT32_C(0x00000002)
#define PS5_PAD_BUTTON_R3          UINT32_C(0x00000004)
#define PS5_PAD_BUTTON_OPTIONS     UINT32_C(0x00000008)
#define PS5_PAD_BUTTON_UP          UINT32_C(0x00000010)
#define PS5_PAD_BUTTON_RIGHT       UINT32_C(0x00000020)
#define PS5_PAD_BUTTON_DOWN        UINT32_C(0x00000040)
#define PS5_PAD_BUTTON_LEFT        UINT32_C(0x00000080)
#define PS5_PAD_BUTTON_L2          UINT32_C(0x00000100)
#define PS5_PAD_BUTTON_R2          UINT32_C(0x00000200)
#define PS5_PAD_BUTTON_L1          UINT32_C(0x00000400)
#define PS5_PAD_BUTTON_R1          UINT32_C(0x00000800)
#define PS5_PAD_BUTTON_TRIANGLE    UINT32_C(0x00001000)
#define PS5_PAD_BUTTON_CIRCLE      UINT32_C(0x00002000)
#define PS5_PAD_BUTTON_CROSS       UINT32_C(0x00004000)
#define PS5_PAD_BUTTON_SQUARE      UINT32_C(0x00008000)
#define PS5_PAD_BUTTON_TOUCH_PAD   UINT32_C(0x00100000)
#define PS5_PAD_BUTTON_INTERCEPTED UINT32_C(0x80000000)

typedef struct ps5_pad_stick {
    uint8_t x;
    uint8_t y;
} ps5_pad_stick_t;

typedef struct ps5_pad_analog_buttons {
    uint8_t l2;
    uint8_t r2;
    uint8_t reserved[2];
} ps5_pad_analog_buttons_t;

typedef struct ps5_pad_quaternion {
    float x;
    float y;
    float z;
    float w;
} ps5_pad_quaternion_t;

typedef struct ps5_pad_vector3 {
    float x;
    float y;
    float z;
} ps5_pad_vector3_t;

typedef struct ps5_pad_touch {
    uint16_t x;
    uint16_t y;
    uint8_t id;
    uint8_t reserved[3];
} ps5_pad_touch_t;

typedef struct ps5_pad_touch_data {
    uint8_t count;
    uint8_t reserved[7];
    ps5_pad_touch_t points[2];
} ps5_pad_touch_data_t;

typedef struct ps5_pad_extension_data {
    uint32_t unit_id;
    uint8_t reserved;
    uint8_t length;
    uint8_t data[10];
} ps5_pad_extension_data_t;

typedef struct ps5_pad_data {
    uint32_t buttons;
    ps5_pad_stick_t left_stick;
    ps5_pad_stick_t right_stick;
    ps5_pad_analog_buttons_t triggers;
    ps5_pad_quaternion_t orientation;
    ps5_pad_vector3_t acceleration;
    ps5_pad_vector3_t angular_velocity;
    ps5_pad_touch_data_t touch;
    int32_t connected;
    uint64_t timestamp_us;
    ps5_pad_extension_data_t extension;
    uint8_t connected_count;
    uint8_t reserved[2];
    uint8_t device_unique_data_length;
    uint8_t device_unique_data[12];
} ps5_pad_data_t;

typedef struct ps5_pad_touchpad_information {
    float pixel_density;
    uint16_t resolution_x;
    uint16_t resolution_y;
} ps5_pad_touchpad_information_t;

typedef struct ps5_pad_stick_information {
    uint8_t dead_zone_left;
    uint8_t dead_zone_right;
} ps5_pad_stick_information_t;

typedef enum ps5_pad_device_class {
    PS5_PAD_DEVICE_CLASS_INVALID = -1,
    PS5_PAD_DEVICE_CLASS_STANDARD = 0,
    PS5_PAD_DEVICE_CLASS_GUITAR = 1,
    PS5_PAD_DEVICE_CLASS_DRUM = 2,
    PS5_PAD_DEVICE_CLASS_DJ_TURNTABLE = 3,
    PS5_PAD_DEVICE_CLASS_DANCE_MAT = 4,
    PS5_PAD_DEVICE_CLASS_NAVIGATION = 5,
    PS5_PAD_DEVICE_CLASS_STEERING_WHEEL = 6,
    PS5_PAD_DEVICE_CLASS_STICK = 7,
    PS5_PAD_DEVICE_CLASS_FLIGHT_STICK = 8,
    PS5_PAD_DEVICE_CLASS_GUN = 9,
} ps5_pad_device_class_t;

typedef struct ps5_pad_controller_information {
    ps5_pad_touchpad_information_t touchpad;
    ps5_pad_stick_information_t sticks;
    uint8_t connection_type;
    uint8_t connected_count;
    int32_t connected;
    int32_t device_class;
    uint8_t reserved[8];
} ps5_pad_controller_information_t;

typedef struct ps5_pad_vibration {
    uint8_t large_motor;
    uint8_t small_motor;
} ps5_pad_vibration_t;

typedef struct ps5_pad_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t reserved;
} ps5_pad_color_t;

typedef enum ps5_pad_trigger_effect_mode {
    PS5_PAD_TRIGGER_EFFECT_OFF = 0,
    PS5_PAD_TRIGGER_EFFECT_FEEDBACK = 1,
    PS5_PAD_TRIGGER_EFFECT_WEAPON = 2,
    PS5_PAD_TRIGGER_EFFECT_VIBRATION = 3,
} ps5_pad_trigger_effect_mode_t;

typedef struct ps5_pad_trigger_effect_command {
    int32_t mode;
    int32_t reserved;
    uint8_t data[48];
} ps5_pad_trigger_effect_command_t;

typedef struct ps5_pad_trigger_effect {
    uint8_t trigger_mask;
    uint8_t reserved[7];
    ps5_pad_trigger_effect_command_t l2;
    ps5_pad_trigger_effect_command_t r2;
} ps5_pad_trigger_effect_t;

typedef enum ps5_pad_trigger_effect_status {
    PS5_PAD_TRIGGER_STATUS_INTERCEPTED = -1,
    PS5_PAD_TRIGGER_STATUS_OFF = 0,
    PS5_PAD_TRIGGER_STATUS_FEEDBACK_STANDBY = 1,
    PS5_PAD_TRIGGER_STATUS_FEEDBACK_ACTIVE = 2,
    PS5_PAD_TRIGGER_STATUS_WEAPON_STANDBY = 3,
    PS5_PAD_TRIGGER_STATUS_WEAPON_PULLING = 4,
    PS5_PAD_TRIGGER_STATUS_WEAPON_FIRED = 5,
    PS5_PAD_TRIGGER_STATUS_VIBRATION_STANDBY = 6,
    PS5_PAD_TRIGGER_STATUS_VIBRATION_ACTIVE = 7,
} ps5_pad_trigger_effect_status_t;

typedef struct ps5_pad_trigger_effect_state {
    int32_t l2;
    int32_t r2;
} ps5_pad_trigger_effect_state_t;

enum {
    PS5_PAD_TRIGGER_MASK_L2 = 1,
    PS5_PAD_TRIGGER_MASK_R2 = 2,
    PS5_PAD_VIBRATION_MODE_ADVANCED = 1,
    PS5_PAD_VIBRATION_MODE_COMPATIBLE = 2,
};

int32_t sceUserServiceInitialize(void *parameters);
int32_t sceUserServiceGetInitialUser(int32_t *user_id);
int32_t sceUserServiceTerminate(void);

int32_t scePadInit(void);
int32_t scePadOpen(int32_t user_id, int32_t port_type, int32_t index,
                   const void *parameters);
int32_t scePadGetHandle(int32_t user_id, int32_t port_type, int32_t index);
int32_t scePadClose(int32_t handle);
int32_t scePadReadState(int32_t handle, ps5_pad_data_t *data);
int32_t scePadRead(int32_t handle, ps5_pad_data_t *data, int32_t capacity);
int32_t scePadGetControllerInformation(
    int32_t handle, ps5_pad_controller_information_t *information);
int32_t scePadSetMotionSensorState(int32_t handle, bool enabled);
int32_t scePadSetTiltCorrectionState(int32_t handle, bool enabled);
int32_t scePadSetAngularVelocityDeadbandState(int32_t handle, bool enabled);
int32_t scePadResetOrientation(int32_t handle);
int32_t scePadSetVibration(int32_t handle,
                           const ps5_pad_vibration_t *vibration);
int32_t scePadSetVibrationMode(int32_t handle, int32_t mode);
int32_t scePadSetLightBar(int32_t handle, const ps5_pad_color_t *color);
int32_t scePadResetLightBar(int32_t handle);
int32_t scePadSetTriggerEffect(int32_t handle,
                               const ps5_pad_trigger_effect_t *effect);
int32_t scePadGetTriggerEffectState(
    int32_t handle, ps5_pad_trigger_effect_state_t *state);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(ps5_pad_data_t) == PS5_PAD_SAMPLE_SIZE,
               "ps5_pad_data_t must be 120 bytes");
_Static_assert(offsetof(ps5_pad_data_t, touch) == 0x34,
               "touch offset must be 0x34");
_Static_assert(offsetof(ps5_pad_data_t, connected) == 0x4c,
               "connected offset must be 0x4c");
_Static_assert(offsetof(ps5_pad_data_t, timestamp_us) == 0x50,
               "timestamp offset must be 0x50");
_Static_assert(sizeof(ps5_pad_controller_information_t) == 28,
               "controller information must be 28 bytes");
_Static_assert(sizeof(ps5_pad_trigger_effect_command_t) == 56,
               "trigger command must be 56 bytes");
_Static_assert(sizeof(ps5_pad_trigger_effect_t) == 120,
               "trigger effect must be 120 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif
