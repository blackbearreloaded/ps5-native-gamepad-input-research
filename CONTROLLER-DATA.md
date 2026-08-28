# Controller data format

## Normal sample

`scePadRead` format zero writes a 120-byte record. The declaration in
[`include/ps5_pad.h`](include/ps5_pad.h) has compile-time size and offset checks.

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| `0x00` | 4 | `buttons` | Digital button mask and interception flag |
| `0x04` | 2 | `left_stick` | X/Y, 0-255, center 128 |
| `0x06` | 2 | `right_stick` | X/Y, 0-255, center 128 |
| `0x08` | 4 | `triggers` | L2/R2 travel, 0-255, then two reserved bytes |
| `0x0C` | 16 | `orientation` | Quaternion X/Y/Z/W |
| `0x1C` | 12 | `acceleration` | Three floats, observed as acceleration in G |
| `0x28` | 12 | `angular_velocity` | Three floats, observed as radians/second |
| `0x34` | 24 | `touch` | Count, reserved bytes, two 8-byte contacts |
| `0x4C` | 4 | `connected` | 32-bit boolean |
| `0x50` | 8 | `timestamp_us` | Controller sample time in process-time microseconds |
| `0x58` | 16 | `extension` | Extension unit ID, length, and up to 10 data bytes |
| `0x68` | 1 | `connected_count` | Changes when a device attaches to the handle |
| `0x69` | 2 | reserved | Preserve/ignore |
| `0x6B` | 1 | unique length | Number of valid device-class bytes, maximum 12 |
| `0x6C` | 12 | unique data | Specialized-device payload |

### Button mask

| Bit | Value | Control |
|---:|---:|---|
| 0 | `0x00000001` | Create/Share |
| 1 | `0x00000002` | L3 |
| 2 | `0x00000004` | R3 |
| 3 | `0x00000008` | Options |
| 4 | `0x00000010` | D-pad Up |
| 5 | `0x00000020` | D-pad Right |
| 6 | `0x00000040` | D-pad Down |
| 7 | `0x00000080` | D-pad Left |
| 8 | `0x00000100` | L2 digital threshold |
| 9 | `0x00000200` | R2 digital threshold |
| 10 | `0x00000400` | L1 |
| 11 | `0x00000800` | R1 |
| 12 | `0x00001000` | Triangle |
| 13 | `0x00002000` | Circle |
| 14 | `0x00004000` | Cross |
| 15 | `0x00008000` | Square |
| 20 | `0x00100000` | Touch-pad click |
| 31 | `0x80000000` | System intercepted the pad |

The PS button is system-owned and is not exposed as an ordinary application
button in this normalized mask. No microphone-mute button bit has been verified
in the normal record. Do not invent mappings for unused bits.

L2 and R2 appear both as digital threshold bits and as independent 8-bit analog
travel. Use the byte values for throttle/brake or variable actions; use the bits
for simple pressed/held/released actions.

## Sticks

Raw axes use 128 as center. A full-range signed conversion used by the working
streaming adapter is:

```c
int32_t axis = ((int32_t)value - 128) * 256;
axis = clamp(axis, INT16_MIN, INT16_MAX);
```

Invert Y when the destination convention expects positive values upward. The
raw controller convention reports smaller Y toward the top.

For UI navigation, use a dead zone and dominant-axis selection. The working
UI adapter uses low/high thresholds 64/192, picks the axis with the larger
distance from center, waits 350 ms before held-stick repeat, and repeats every
110 ms. It uses the left stick only in the final app.

For gameplay, radial dead-zone processing generally preserves diagonals better
than the UI's dominant-axis rule. Query `scePadGetControllerInformation` for the
device-reported left/right dead-zone bytes, then apply application-specific
response curves after centering and normalization.

## Button edges

For each chronological sample:

```c
uint32_t changed  = previous ^ current;
uint32_t pressed  = changed & current;
uint32_t released = changed & previous;
previous = current;
```

This captures a complete press/release pair even when both occurred between
rendered frames, provided both records remain in the returned batch. If more
than 64 records accumulated, the library drops older records and returns the
newest 64.

## Touch pad

The record contains zero, one, or two active contacts. Each contact has:

- 16-bit X and Y coordinates;
- an 8-bit ID that remains stable while that finger stays down;
- three reserved bytes.

Clamp count to two before iterating. Use contact IDs—not array position alone—to
track begin/move/end transitions. Query controller information for the touch
resolution and normalize with `x / resolution_x`, `y / resolution_y`.

The touch-pad physical click is a separate digital button bit. Touch contact
does not imply click and click does not imply an active contact.

## Motion

Motion fields contain:

- orientation quaternion X/Y/Z/W;
- acceleration X/Y/Z;
- angular velocity X/Y/Z.

Motion sensors are enabled by default in observed application behavior. The app can
explicitly enable them, enable tilt correction, enable angular-velocity
deadband, and reset orientation. Resetting orientation treats the controller's
current attitude as identity.

Integrate or filter samples using `timestamp_us`, not render-frame time. Keep
quaternion normalization and coordinate-system conversion in the application;
the exact gameplay coordinate convention still needs a dedicated physical
motion fixture.

## Time and connection

The 64-bit timestamp is copied from each raw driver record. The fallback path
uses `sceKernelGetProcessTime() - 1000`, supporting a process-time microsecond
clock domain. It is suitable for ordering and relative sample-age measurement;
do not treat it as wall-clock time.

`connected_count` is a generation counter. If it changes, clear edge history,
touch IDs, calibration, pending long-press/gesture state, synthesized output
buttons, and any device-specific cached information. Invalidate state-send
deduplication so the first neutral or restored state of the new generation is
delivered. A handle can remain valid while its physical controller changes.

## Extension and device-class data

The final 32 bytes support extension units and specialized devices. Normal
DualSense gameplay does not need them. `scePadDeviceClassParseData` recognizes
standard, guitar, drums, turntable, dance mat, navigation, steering wheel,
arcade stick, flight stick, and gun classes. Only the steering-wheel payload
has been independently documented in detail; do not decode other class
payloads by guesswork.
