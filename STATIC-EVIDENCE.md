# Static compatibility evidence

This document records the functional compatibility conclusions used by the
examples. It intentionally omits proprietary binaries and excerpts, binary
fingerprints, analysis-tool metadata, internal code addresses, private
workspace locations, and acquisition details.

The declarations in [`include/ps5_pad.hpp`](include/ps5_pad.hpp) are independently
authored for interoperability. Their structure sizes and offsets are checked by
[`tests/layout_check.cpp`](tests/layout_check.cpp), and application behavior is
summarized in [VALIDATION.md](VALIDATION.md).

## Normal Pad input

The application-facing read contract has these observable properties:

- `scePadRead` accepts capacities from 1 through 64 records.
- A read performs one platform-driver request and returns zero or more new
  normalized records.
- When more records are available than fit, the newest records are retained.
- Retained records preserve chronological order.
- Normal records are 120 bytes each.
- The final emitted record becomes the latest state used by current-state
  polling.
- `scePadReadState` returns the latest normalized state and can reuse cached
  state when no new record is available.
- The normal hot path requires no per-call heap allocation.
- Reads are serialized inside the library, so one owner per handle avoids
  unnecessary contention and ambiguous queue consumption.

These properties explain why maximum-capacity batched reads preserve short
transitions better than latest-state polling without requiring raw transport
access.

## Normal record layout

The independently documented 120-byte record is:

| Offset | Field | Type/size |
|---:|---|---|
| `0x00` | `buttons` | `uint32_t` |
| `0x04` | `left_stick` | 2 bytes |
| `0x06` | `right_stick` | 2 bytes |
| `0x08` | `triggers` | 4 bytes |
| `0x0C` | `orientation` | four floats |
| `0x1C` | `acceleration` | three floats |
| `0x28` | `angular_velocity` | three floats |
| `0x34` | `touch` | 24 bytes, up to two contacts |
| `0x4C` | `connected` | 32-bit state |
| `0x50` | `timestamp_us` | `uint64_t` |
| `0x58` | `extension` | 16 bytes |
| `0x68` | `connected_count` | `uint8_t` generation value |
| `0x6B` | `device_unique_data_length` | `uint8_t` |
| `0x6C` | `device_unique_data` | 12 bytes |

Nested contracts establish two 8-byte touch records, two trigger bytes, a
quaternion, acceleration and angular-velocity vectors, and extension data.
Controller information is 28 bytes. Basic vibration and light-bar parameters
are 2 and 4 bytes.

## Application-facing control surface

The documented normal-application surface includes:

| Export | Role |
|---|---|
| `scePadSetMotionSensorState` | Enable or disable motion updates |
| `scePadSetTiltCorrectionState` | Configure orientation drift correction |
| `scePadSetAngularVelocityDeadbandState` | Configure resting gyro suppression |
| `scePadResetOrientation` | Recenter orientation |
| `scePadSetVibration` | Set two vibration levels |
| `scePadSetVibrationMode` | Select the supported vibration mode |
| `scePadSetLightBar` | Set an RGB light-bar value |
| `scePadResetLightBar` | Restore platform light behavior |
| `scePadGetControllerInformation` | Read touch, stick, connection, and class information |
| `scePadSetTriggerEffect` | Configure adaptive-trigger effects |
| `scePadGetTriggerEffectState` | Read both trigger-effect states |

These calls retain platform policy and transport handling. Applications should
prefer them to constructing raw USB or Bluetooth reports.

## Related libraries

`libSceHidControl` exposes specialized device and service controls. Its state
interfaces do not provide the normal controller's normalized, timestamped batch
queue and are not a demonstrated low-latency replacement for `scePadRead`.

`libSceBluetoothHid` exposes raw Bluetooth HID transport responsibilities such
as registration, callbacks, descriptors, and report requests. Direct use would
require transport lifecycle handling and model-specific report parsing, and it
would not naturally cover USB-connected controllers. No observed result shows
a latency advantage over the normal Pad interface.

## Confidence boundary

Buttons, both sticks, analog triggers, lifecycle behavior, batched ordering,
and connection handling are device-tested. Touch and motion layouts are
contract-tested but still need dedicated physical fixtures. Vibration,
lighting, and adaptive-trigger declarations compile and pass layout checks but
remain explicitly marked as pending physical verification.
