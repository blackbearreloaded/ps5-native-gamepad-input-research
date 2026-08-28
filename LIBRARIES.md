# Library responsibilities

## Decision table

| Library | Role | Use in an ordinary app? |
|---|---|---|
| `libSceUserService` | Resolve the signed-in user that owns a standard pad | Yes |
| `libScePad` | Normalized input queue, controller information, motion controls, feedback | Yes |
| `libSceHidControl` | Specialized device/service control | No, unless targeting its named devices |
| `libSceBluetoothHid` | Raw Bluetooth HID registration, reports, callbacks, transport lifecycle | No |

## `libSceUserService`

Only three functions are needed for the common single-user lifecycle:

- `sceUserServiceInitialize(nullptr)`;
- `sceUserServiceGetInitialUser(&user_id)`;
- `sceUserServiceTerminate()` when this component owns initialization.

The controller must be opened for the real signed-in user. The system user is
not a shortcut for the active DualSense.

## `libScePad`

This is the application API. Its relevant exports fall into five groups.

### Normal application path

- `scePadInit`
- `scePadOpen`, `scePadGetHandle`, `scePadClose`
- `scePadRead`, `scePadReadState`
- `scePadGetControllerInformation`
- `scePadSetMotionSensorState`
- `scePadSetTiltCorrectionState`
- `scePadSetAngularVelocityDeadbandState`
- `scePadResetOrientation`
- `scePadSetVibration`, `scePadSetVibrationMode`
- `scePadSetLightBar`, `scePadResetLightBar`
- `scePadSetTriggerEffect`, `scePadGetTriggerEffectState`

These functions are declared in [`include/ps5_pad.hpp`](include/ps5_pad.hpp).

### Alternative read formats

- `scePadReadExt` / `scePadReadStateExt` use the same common read core with a
  128-byte output record and an additional capability check.
- `scePadUiReadState` requests a 168-byte system/UI format and repacks it.
- `scePadReadHistory` allocates a large temporary buffer and requests a fixed
  2,400-record history.
- tracker and VR-controller reads have different contracts.

None is a lower-latency replacement for normal `scePadRead`. The extra eight
bytes in Ext format are not yet mapped, so Ext is intentionally absent from the
public example header.

### Specialized device classes and extension ports

The interface includes device-class information/parsing, extension unit
information, feature reports, extension reports, VR tracking, and
special-device opens. Use
them only with a verified structure for the exact device class. The normal
sample already carries up to 12 unique bytes for class parsing.

### System/service control

Exports for connecting/disconnecting ports, process focus/privilege,
interception, user colors/numbers, power saving, USB switching, transport
buttons, remote controls, virtual devices, and shared output data belong to
platform services or privileged tooling. They are unnecessary for app input.

### Maintenance and tuning

The library also contains controller/bond firmware update, profile, saved
setting, stick/trigger tuning, recording, feature-report, and device-management
exports. These are not a game API. Their ABIs and privilege contracts are not
established here; calling them can change controller or system state.

## `libSceHidControl`

The observed interface includes:

- port/handle connect and disconnect;
- device information, name, ID, battery, auth, and version;
- process/application focus and handle state;
- specialized state, extension, audio, and volume controls;
- microphone beam forming, user color, force-update and weak-feedback controls.

The specialized state getters do not expose the general DualSense normalized
queue, batching, or timestamp behavior. No evidence shows lower latency than
Pad.

Use HidControl only for a separately documented specialized-device requirement.
Do not add it to a standard controller app.

## `libSceBluetoothHid`

The observed interface includes initialization, callback registration, device
registration, descriptors, input/feature report requests, output reports,
device information/name, disconnect, and unregister.

This is a raw Bluetooth transport component. Direct use would require:

- pairing/registration and service lifecycle;
- HID report-descriptor parsing;
- model- and firmware-specific input report parsing;
- raw output/feature report construction;
- callback/thread synchronization;
- a separate path for USB-connected controllers;
- reimplementation of policy already handled by Pad.

This would add complexity and can reduce compatibility without evidence of a
latency benefit. Use `libScePad` for controller input regardless of whether the
physical transport is Bluetooth or USB.

## Selection rule

If the requirement can be described as “use the player's controller in an
application,” select UserService + Pad. HidControl and BluetoothHid become
relevant only when the requirement names a specialized device or raw transport
operation that Pad does not provide and the corresponding contract has been
independently established.
