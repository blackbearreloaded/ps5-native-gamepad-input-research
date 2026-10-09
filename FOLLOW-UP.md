# Runtime Validation Plan

## Already completed

- Standard UserService/Pad initialization, open, repeated reads, and close are
  device-tested in both UI and streaming applications.
- Full joystick behavior is developer-confirmed in both referenced apps.
- The streaming lifecycle run sustained approximately 243 Hz current-state
  polling with zero read errors.
- The UI adapter uses the 64-record batch and has host regressions for button
  edges, left-stick dead zone, dominant-axis selection, and repeat behavior.
- The native keyboard/mouse test application physically captured keyboard HID usages plus mouse motion, primary/secondary buttons,
  vertical wheel, and horizontal tilt from one wireless combination receiver.
- The same open keyboard and mouse handles detected receiver removal and
  resumed non-neutral input after reconnection without reinitialization.

The remaining plan focuses on quantitative sample timing and the features
that have not yet received a dedicated physical fixture.

## Minimal probe

Adapt the existing native pad probe to:

1. Allocate 64 records (`64 * 120` bytes).
2. Call `scePadRead(handle, records, 64)` in the test loop.
3. Record the process time immediately before and after each call.
4. For every returned record, log its timestamp, buttons, sticks, triggers, connection flag, and position in the batch.
5. Keep aggregate counters in memory and emit summaries periodically rather than logging every empty poll.

## Metrics

- Samples returned per call, including zero-result calls.
- Delta between consecutive controller sample timestamps.
- `time_after_read - sample_timestamp` for the final sample in each batch.
- read-call duration.
- Backlog depth and frequency of multi-sample batches.
- Button-edge preservation when the application deliberately stalls for known intervals.
- p50, p95, p99, and maximum sample age and call duration.

## Test matrix

| Variable | Cases |
|---|---|
| Transport | Bluetooth; USB data connection |
| App cadence | 60 Hz; 120 Hz; dedicated reader |
| Batch capacity | 1; 8; 64 |
| Load | Idle; CPU-heavy frame; GPU-heavy frame |
| Focus | Foreground; system UI interception if safely testable |
| Input type | Buttons; analog sweep; touch; motion sensor |

## Decision criteria

- Keep frame-driven `scePadRead(..., 64)` if its final-sample age is already within the latency target.
- Add a dedicated reader only if frame cadence is visibly dominating sample age.
- Prefer USB only if measurement shows a meaningful improvement for the intended controller and configuration.

## Keyboard and mouse

- Observe modifier keys and lock-LED state with dedicated keyboard input.
- Observe additional mouse buttons with hardware that exposes them.
- Attach separate same-class devices and verify simultaneous independent
  indexes.
- Measure device timestamps and queue depths separately from application poll
  cadence.

## Gamepad

- Touch contacts and coordinate range with a dedicated physical test.
- Motion units and coordinate convention with a dedicated physical test.
