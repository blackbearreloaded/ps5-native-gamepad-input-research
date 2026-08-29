# Library responsibilities

## Decision table

| Library | Role | Use in an ordinary app? |
|---|---|---|
| `libSceUserService` | Resolve the signed-in user that owns a standard pad | Yes |
| `libScePad` | Normalized input queue, controller information, motion controls, feedback | Yes |
| `libSceKeyboard` | Normalized keyboard queue and current state | Yes, for physical keyboards |
| `libSceMouse` | Relative mouse queue, buttons, wheels, and optional two-index merge | Yes, for physical mice |
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

## `libSceKeyboard`

This is the normal application path for physical keyboard capture. The useful
subset is initialization, signed-in-user open/get-handle, batched read,
current-state read, connection query, and close. It exposes timestamped HID
usage sets, left/right modifier bits, LED state, connection, and interception.

The tested integration first loads sysmodule ID `0x0106`, then calls
`sceKeyboardInit` and opens the selected index for the initial user. A
wireless combination receiver produced keyboard reports on index 1 in the
device test. Index assignment is environment-specific, so discover it instead
of assuming that every keyboard is index 0.

Use the batched read for press/release fidelity. Character composition and
layout mapping are separate from the native key queue. See
[Native keyboard and mouse input](KEYBOARD-MOUSE.md).

## `libSceMouse`

This is the normal application path for physical mouse capture. The useful
subset is initialization, signed-in-user open/get-handle, batched read, pointer
speed/hand settings, and close. It exposes timestamped relative X/Y motion,
vertical and horizontal wheels, five buttons, connection, and interception.

It can open one device index or merge indexes 0 and 1. Neither mode requires
raw HID parsing or Bluetooth registration. See
[Native keyboard and mouse input](KEYBOARD-MOUSE.md).

The tested integration loads sysmodule ID `0x00a9` before initialization. The
mouse interface of the same combination receiver produced reports on index 0,
independently of its keyboard interface on index 1.

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

Select UserService + Pad for a gamepad, UserService + Keyboard for a physical
keyboard, and UserService + Mouse for a physical mouse. HidControl and
BluetoothHid become relevant only when the requirement names a specialized
device or raw transport operation that the normal application libraries do not
provide and the corresponding contract has been independently established.
