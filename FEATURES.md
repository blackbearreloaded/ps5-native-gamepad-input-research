# Controller features and outputs

## Controller information

Call after opening a handle and whenever `connected_count` changes:

```c
ps5_pad_controller_information_t information = {0};
int result = scePadGetControllerInformation(handle, &information);
```

The 28-byte result provides touch-pad density/resolution, reported stick dead
zones, connection type, connection generation, connection state, and device
class. The implementation zeroes the structure before filling it and can return
success with `connected = 0`.

Known device classes are standard controller, guitar, drums, DJ turntable,
dance mat, navigation controller, steering wheel, arcade stick, flight stick,
and gun. Standard DualSense apps normally require only class zero.

## Motion configuration

```c
scePadSetMotionSensorState(handle, true);
scePadSetTiltCorrectionState(handle, true);
scePadSetAngularVelocityDeadbandState(handle, true);
scePadResetOrientation(handle);
```

- Motion reporting is on by default.
- Tilt correction is off by default and corrects orientation drift.
- Angular-velocity deadband is off by default and suppresses resting noise.
- Reset orientation recenters the current attitude.

Check every return value. A device class without motion sensors may reject these
calls. See [`examples/05-motion-touch.c`](examples/05-motion-touch.c).

## Vibration

The basic output has two unsigned 8-bit levels:

```c
ps5_pad_vibration_t vibration = {
    .large_motor = 180,
    .small_motor = 80,
};
scePadSetVibration(handle, &vibration);
```

The names reflect the API's compatibility model: the large/left level is the
low-frequency component and the small/right level is the high-frequency
component. Stop by sending `{0, 0}`. Always stop vibration during shutdown and
when the application loses ownership of a remote session.

`scePadSetVibrationMode` selects advanced mode `1` or compatible mode `2` in the
documented normal-app declaration. Do not switch modes repeatedly in the input
hot path.

## Light bar

```c
ps5_pad_color_t blue = {.r = 0, .g = 80, .b = 255};
scePadSetLightBar(handle, &blue);
```

The color is RGB with one zero reserved byte. Restore platform/default behavior
with `scePadResetLightBar(handle)`. The implementation can reject unsupported
controllers and certain invalid colors, so treat a negative result as a normal
feature-availability failure.

## Adaptive triggers

The 120-byte trigger parameter has a mask followed by 56-byte commands for L2
and R2. A clear mask bit leaves that trigger unchanged.

### Off

Mode `0`. Zero the command. Apply to both masks to release both triggers.

### Feedback

Mode `1`:

- `data[0]`: position, 0-9;
- `data[1]`: strength, 0-8; zero means no effect.

### Weapon

Mode `2`:

- `data[0]`: start position, 2-7;
- `data[1]`: end position, greater than start and at most 8;
- `data[2]`: strength, 0-8.

The trigger resists across the span and gives way past the end.

### Vibration

Mode `3`:

- `data[0]`: position, 0-9;
- `data[1]`: amplitude, 0-8;
- `data[2]`: frequency in hertz.

Build a zero-initialized command, validate these ranges in application code,
set only the required bytes, and call `scePadSetTriggerEffect`. See
[`examples/06-feedback.c`](examples/06-feedback.c).

`scePadGetTriggerEffectState` returns one integer state per trigger: intercepted
`-1`, off `0`, feedback standby/active `1`/`2`, weapon standby/pulling/fired
`3`/`4`/`5`, or vibration standby/active `6`/`7`. The names and values are
declared in `ps5_pad_trigger_effect_status_t`; this repository has not yet
hardware-validated the transitions.

## Touch and motion output to another protocol

When forwarding a DualSense to a remote host:

- advertise only features actually forwarded;
- send controller arrival before state packets;
- preserve touch IDs and normalize coordinates using controller information;
- preserve timestamps for motion rate control;
- send neutral/removal before teardown;
- map host feedback callbacks back to vibration, light-bar, and trigger APIs
  only after validating each output on hardware.

The device-tested streaming milestone advertised the applicable controller
type and analog triggers only. It deliberately did not claim touch, motion, rumble, LED,
or adaptive-trigger support because those feedback bridges were not yet present.

The later production client confirmed that this lifecycle remains reliable
with batched Pad reads and local input multiplexing. It also established the
required transition rules for controller-to-mouse mode: neutralize the virtual
gamepad before switching, keep it alive with neutral packets, release all
synthesized mouse buttons on every exit path, and force the first gamepad state
through deduplication when switching back. See
[Production integration findings](PRODUCTION-INTEGRATION.md).

## Features not exposed by the normal path

The PS/system button is owned by the operating system. Microphone capture,
speaker audio, headset routing, and the mute button are not part of the normal
120-byte controller record. They involve separate audio/device services and are
not documented as controller input here.

Do not use raw Bluetooth output reports for vibration or triggers merely because
the transport library exports them. `libScePad` already handles USB/Bluetooth
transport differences, device policy, report construction, and handle locking.
