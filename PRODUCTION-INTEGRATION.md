# Production integration findings

This document records controller lessons learned after the original native
proof was integrated into a complete streaming client. It supplements the Pad
contract documentation with behavior observed under real video, audio,
reconnect, shortcut, and mouse-emulation workloads.

Evidence comes from application telemetry and repeated testing with a physical
DualSense. Private source locations, task identifiers, commit identifiers, and
raw logs are intentionally excluded from the public repository.

## What changed after the first proof

The first streaming adapter used `scePadReadState` every 4 ms. It proved the
signed-in-user lifecycle, full joystick mapping, controller arrival/removal,
change suppression, and a one-second keepalive.

The production client later replaced the cached-state call with:

```cpp
std::array<ps5::pad::Data, ps5::pad::kMaxSamples> samples{};
const auto count = scePadRead(
    handle, samples.data(), static_cast<std::int32_t>(samples.size()));
```

This change followed a long gameplay report where input eventually stopped.
That single report did not prove that cached-state polling was the root cause,
but it exposed two weaknesses in the old integration: intermediate transitions
could be lost, and the telemetry could not distinguish empty reads, queued
samples, interception, generation changes, read failures, and protocol-send
failures.

After the batched path was deployed, the developer reported substantially
better repeated-stream behavior. Later telemetry recorded active controller and
mouse-mode sessions with zero Pad read, controller-send, and mouse-send errors.

## Batched reads matter even at a 4 ms poll

The production loop still sleeps 4 ms after each poll. Nevertheless, real
sessions returned as many as seven records in one `scePadRead` call:

| Polls | Samples | Empty reads | Maximum batch | Forwarded events | Intercepted samples | Mouse toggles/moves/buttons | Errors |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 582 | 1,339 | 31 | 7 | 23 | 0 | 0 / 0 / 0 | 0 |
| 3,464 | 8,380 | 130 | 6 | 1,418 | 0 | 2 / 24 / 0 | 0 |
| 6,520 | 15,625 | 248 | 5 | 3,963 | 983 | 4 / 22 / 2 | 0 |
| 8,357 | 21,079 | 224 | 5 | 783 | 0 | 2 / 12 / 2 | 0 |
| 43,792 | 19,716 | 33,624 | 3 | 18,946 | 0 | 0 / 0 / 0 | 0 |

The count can exceed one because scheduling, video/audio work, and controller
report timing do not stay phase-locked to the 4 ms application loop. Processing
only one latest state would discard observable transitions in these sessions.

An empty read is also normal. It means no new queued record was available, not
that the handle failed. Preserve the last valid analog state for current-state
uses, continue any required protocol keepalive, and do not manufacture button
edges.

## Production read-loop rules

For every successful batch:

1. Process records from index zero through `count - 1`.
2. Detect a `connected_count` change before applying edge or gesture state.
3. Treat disconnected and system-intercepted records as neutral.
4. Evaluate local shortcut edges against the chronological raw records.
5. Map and forward each retained transition.
6. Keep the final record as the current analog state.

When `connected_count` changes, the production client resets its previous raw
button mask and pending Options long-press, releases synthesized mouse buttons,
and starts the new controller generation without inheriting state from the old
device. Applications should also clear touch IDs, calibration, and other
device-specific caches.

The client records at least these counters per session:

- polls, samples, empty reads, and maximum batch;
- current connection generation;
- disconnected and intercepted samples;
- non-neutral samples and observed raw/mapped button masks;
- controller arrival, state-send, and removal results;
- mouse mode toggles, moves, button/scroll events, and send errors.

These counters are cheap and distinguish Pad acquisition failures from protocol
or presentation problems without logging every sample.

## Streaming protocol lifecycle

The working streaming integration follows this order:

```text
open signed-in user's standard pad
  -> announce controller slot 0 with its supported controller type
  -> advertise only the supported buttons and analog triggers
  -> drain ordered Pad batches
  -> send changed controller states
  -> send an unchanged state at least once per second
  -> send neutral/inactive controller state
  -> stop the streaming connection
  -> close the Pad handle
```

The capability declaration matters. The client advertises the applicable
controller type and analog triggers because those fields are forwarded. It does
not advertise touch coordinates, motion sensors, rumble, light bar, or adaptive
triggers until the corresponding bidirectional protocol bridges exist.

The mapping used by the working client is:

| DualSense | Streaming-protocol field |
| --- | --- |
| Cross / Circle / Square / Triangle | A / B / X / Y |
| D-pad | Up / Down / Left / Right |
| L1 / R1 | LB / RB |
| L3 / R3 | Left/right stick click |
| Options | Play/Start |
| Touch-pad click | Touchpad/Select |
| L2 / R2 | Independent 8-bit analog triggers |
| Both sticks | Signed 16-bit axes with Y inverted |

Create/Share is not forwarded by this integration. The mapping is a protocol
choice, not a different native button definition.

## Consume local chords before forwarding

The client uses touch-pad click as the protocol's Select button. Its simplified
local chords are:

- Touchpad + L1: disconnect the stream and return to the launcher;
- Touchpad + R1: toggle the local metrics overlay.

Detect each chord on its transition, perform the local action once, and remove
the consumed bits before constructing an upstream controller packet. Otherwise
the game receives unintended touchpad/shoulder presses as a side effect of a
client command.

## Controller-to-mouse mode

The production client also demonstrates safely multiplexing one physical pad
between two virtual input devices:

- hold Options for more than 750 ms and release to toggle mode;
- a short Options press remains an ordinary Start press;
- use Pad sample timestamps for the hold duration, falling back to monotonic
  process time only when the sample timestamp is zero;
- choose the stick with the larger combined X/Y deflection;
- update relative pointer motion every 50 ms with a cubic response curve;
- map Cross/Circle/Square to left/right/middle click;
- map L1/R1 to mouse buttons 4/5;
- map D-pad edges to vertical/horizontal wheel clicks.

Mouse mode was physically exercised and exposed a transition bug: after
switching modes, the host could retain the mouse while its virtual controller
stopped receiving usable state. The corrected invariants are:

1. Send a neutral gamepad state before mouse mode takes ownership.
2. Continue neutral gamepad keepalives while emitting mouse input so the host
   does not retire the virtual controller.
3. Release every synthesized mouse button on mode exit, disconnect, controller
   generation change, and stream teardown.
4. Invalidate controller-state deduplication when returning to gamepad mode so
   the first restored state is sent even when it equals a cached state.
5. Show a visible notification naming the active mode.

Post-fix telemetry observed two- and four-toggle sessions, pointer motion,
mouse-button events, and zero mouse-send errors. Vertical/horizontal scroll is
implemented and host-tested, but the cited telemetry did not record a physical
scroll event.

## Acquisition latency is not motion-to-photon latency

The client polls Pad in a 4 ms loop separate from the video decode callbacks.
During streaming tests the Pad and protocol counters could remain healthy while
the image appeared delayed or stalled. In that situation, likely causes include
host capture/encoding delay, packet loss and recovery, queued or late video,
missing a display vblank, and television processing.

Do not add controller buffering to compensate for delayed video; it increases
real input latency. Compare the host monitor with the client display:

- immediate host response but late client response points to video/presentation;
- late response on both displays points to host/game processing;
- increasing Pad read or protocol-send errors points to the input path.

For quantitative latency work, record Pad poll interval p95/p99/max and send
duration, then pair a known button edge with a visible flash captured by a
high-frame-rate camera. Controller poll counters alone do not measure the full
button-to-photon path.

## Remaining production gap

The current integration counts negative Pad reads but does not yet reopen a pad
handle after repeated failures. A production recovery policy should publish
neutral state first, close and reopen the same signed-in user's standard pad,
reset all generation/edge/mode state, and reannounce the protocol controller.
That policy remains a recommended fault-injection experiment rather than a
hardware-accepted requirement.
