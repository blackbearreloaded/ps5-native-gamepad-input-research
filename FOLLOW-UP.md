# Runtime Validation Plan

## Already completed

- Standard UserService/Pad initialization, open, repeated reads, and close are
  device-tested in both UI and streaming applications.
- Full joystick behavior is developer-confirmed in both referenced apps.
- The streaming lifecycle run sustained approximately 243 Hz current-state
  polling with zero read errors.
- The UI adapter uses the maximum batch and has host regressions for button
  edges, left-stick dead zone, dominant-axis selection, and repeat behavior.
- The native keyboard/mouse probe loaded both sysmodules and physically
  captured keyboard HID usages plus mouse motion, primary/secondary buttons,
  vertical wheel, and horizontal tilt from one wireless combination receiver.

The remaining plan therefore focuses on quantitative sample timing and the
features that have not yet received a dedicated physical fixture.

The current compatibility evidence is sufficient to choose `scePadRead` as the
application hot path. Runtime testing is still required to quantify latency and
controller report behavior on the target system-software version and transport.

## Minimal probe

Adapt the existing native pad probe to:

1. Allocate 64 normal records (`64 * 120` bytes).
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

Capacity 1 through `scePadRead` should be tested separately from `scePadReadState`: both invoke the common core for one record, but the state API has cached fallback behavior when no new report is available.

## Decision criteria

- Keep frame-driven `scePadRead(..., 64)` if its final-sample age is already within the latency target.
- Add a dedicated reader only if frame cadence is visibly dominating sample age.
- Prefer USB only if measurement shows a meaningful improvement for the intended controller and configuration.
- Do not pursue direct `BluetoothHid` access unless `libScePad` fails a measured requirement and raw transport access is proven available to the application process.

## Optional contract work

### Keyboard and mouse

- Confirm left/right modifier bits and LED state with dedicated keyboard input.
- Confirm middle, X1, and X2 mouse buttons with hardware that exposes them.
- Exercise unplug/reconnect and verify neutral edge publication.
- Measure device timestamps and queue depths separately from application poll
  cadence.
- Test merged mouse behavior only after the single-index path passes; use two
  physical mice and verify how corresponding reports are combined.
- Recover and validate text-composition semantics only if an application needs
  the key-to-character helper rather than its own HID layout mapping.

No more library analysis is required for the normal
buttons/sticks/triggers/touch/motion path. Useful optional work is limited to:

- the exact 128-byte `scePadReadExt` structure, if its additional fields are needed;
- motion-sensor enable/reset controls, if the runtime probe will measure IMU data;
- specialized device-class payloads, only when an application has a concrete
  need for one of those devices.

Pairing, Bluetooth registration, and HID service initialization functions are not relevant to this investigation's application hot path.
