# Controller data format

## Gamepad sample

`scePadRead` writes 120-byte records. The declaration in
[`include/ps5_pad.hpp`](include/ps5_pad.hpp) has compile-time size and offset checks.

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| `0x00` | 4 | `buttons` | Digital button mask and interception flag |
| `0x04` | 2 | `left_stick` | X/Y, 0-255, center 128 |
| `0x06` | 2 | `right_stick` | X/Y, 0-255, center 128 |
| `0x08` | 4 | `triggers` | L2/R2 travel, 0-255, then two padding bytes |
| `0x0C` | 16 | `orientation` | Quaternion X/Y/Z/W |
| `0x1C` | 12 | `acceleration` | Three floats |
| `0x28` | 12 | `angular_velocity` | Three floats |
| `0x34` | 24 | `touch` | Count, padding, two 8-byte contacts |
| `0x4C` | 4 | `connected` | Nonzero while a controller is attached |
| `0x50` | 8 | `timestamp_us` | Sample time in microseconds |
| `0x58` | 16 | `opaque0` | Not interpreted here |
| `0x68` | 1 | `connection_generation` | Changes when a controller attaches to the handle |
| `0x69` | 15 | `opaque1` | Not interpreted here |

### Button mask

| Bit | Value | Control |
|---:|---:|---|
| 0 | `0x00000001` | Create |
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

The PS button is system-owned and was not seen as an application button in
this mask. No microphone-mute button bit has been observed. Do not invent
mappings for unused bits.

L2 and R2 appear both as digital threshold bits and as independent 8-bit analog
travel. Use the byte values for throttle/brake or variable actions; use the bits
for simple pressed/held/released actions.

## Sticks

Raw axes use 128 as center. A full-range signed conversion used by the working
streaming adapter is:

```cpp
auto axis = (static_cast<std::int32_t>(value) - 128) * 256;
axis = std::clamp(
    axis,
    static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::min()),
    static_cast<std::int32_t>(std::numeric_limits<std::int16_t>::max()));
```

Invert Y when the destination convention expects positive values upward. The
raw controller convention reports smaller Y toward the top.

For UI navigation, use a dead zone and dominant-axis selection. The working
UI adapter uses low/high thresholds 64/192, picks the axis with the larger
distance from center, waits 350 ms before held-stick repeat, and repeats every
110 ms. It uses the left stick only in the final app.

For gameplay, radial dead-zone processing generally preserves diagonals better
than the UI's dominant-axis rule. Apply application-specific response curves
after centering and normalization.

## Button edges

For each chronological sample:

```cpp
const ps5::pad::ButtonMask changed = previous ^ current;
const auto pressed = changed & current;
const auto released = changed & previous;
previous = current;
```

This captures a complete press/release pair even when both occurred between
rendered frames, provided both records are in the returned batch.

## Touch pad

The record contains zero, one, or two active contacts. Each contact has:

- 16-bit X and Y coordinates;
- an 8-bit ID;
- three padding bytes.

Clamp count to two before iterating. Use contact IDs, not array position
alone, to track begin/move/end transitions. The coordinate range has not been
measured here; establish it on the target controller before normalizing.

The touch-pad physical click is a separate digital button bit.

Touch contacts still need a dedicated physical test.

## Motion

Motion fields contain:

- orientation quaternion X/Y/Z/W;
- acceleration X/Y/Z;
- angular velocity X/Y/Z.

Units and the coordinate convention still need a dedicated physical test.
Integrate or filter samples using `timestamp_us`, not render-frame time.

## Time and connection

`timestamp_us` is suitable for ordering samples and for relative durations;
the tested streaming client uses it to time a 750 ms button hold. Do not treat
it as wall-clock time.

`connection_generation` is a generation counter. If it changes, clear edge
history, touch IDs, calibration, pending long-press/gesture state, synthesized
output buttons, and any device-specific cached information. Invalidate
state-send deduplication so the first neutral or restored state of the new
generation is delivered. A handle can remain valid while its physical
controller changes.
