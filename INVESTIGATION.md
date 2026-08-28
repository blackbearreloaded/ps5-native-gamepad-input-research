# Investigation Report

## Scope

This report covers controller sampling after an application already has a valid pad handle. Pairing, discovery, registration, and service initialization are intentionally out of scope.

## Recommended hot path

Use this shape once per input or simulation iteration:

```c
enum { PAD_SAMPLE_SIZE = 120, PAD_SAMPLE_CAPACITY = 64 };

unsigned char samples[PAD_SAMPLE_SIZE * PAD_SAMPLE_CAPACITY];
int count = scePadRead(handle, samples, PAD_SAMPLE_CAPACITY);

if (count > 0) {
    for (int i = 0; i < count; ++i) {
        const unsigned char *sample = samples + i * PAD_SAMPLE_SIZE;
        process_edges_and_motion(sample);
    }

    use_as_current_state(samples + (count - 1) * PAD_SAMPLE_SIZE);
}
```

The actual application should use an independently documented `ScePadData`
declaration rather than byte offsets. The byte buffer above only makes the
record stride and batching behavior explicit.

Call the read as late as practical before simulation consumes input. This avoids adding a full frame of application-side queueing.

## Why `scePadRead` is the best fit

The exported function is a thin wrapper around the library's common read routine. That routine:

1. Validates a sample capacity from 1 through 64.
2. Takes a process-wide mutex.
3. Submits one request to the already-open platform input driver.
4. Receives up to 64 raw driver records.
5. Selects the newest records that fit the caller's capacity.
6. Converts them into normalized application records.
7. Caches the final returned record as the latest state.
8. Releases the mutex and returns the number of records written.

The normal path has no per-call heap allocation. Conversion work scales with
the number of records returned, while the driver interaction remains one
request per call.

### Queue behavior

The read core computes a skip count equivalent to `max(driver_count - requested_capacity, 0)`. It walks the driver records in sequence and emits records only after that skip count. The returned count is therefore `min(driver_count, requested_capacity)`.

This matters for latency: when an application is temporarily late, the output is biased toward the newest input rather than stale records. The records that remain are returned in their original order, and the final one is copied to the per-handle latest-state cache.

### Timestamp and payload

For normal `ScePadData` records:

| Offset | Size | Observed purpose |
|---:|---:|---|
| `0x00` | 4 | Buttons |
| `0x04` | 4 | Left/right stick bytes |
| `0x08` | 2 | Analog triggers |
| `0x0C` | 16 | Orientation quaternion |
| `0x1C` | 12 | Acceleration |
| `0x28` | 12 | Angular velocity |
| `0x34` | 1 | Touch count |
| `0x3C` | 8 | Touch point 0 |
| `0x44` | 8 | Touch point 1 |
| `0x4C` | 1 | Connected flag |
| `0x50` | 8 | Sample timestamp |
| `0x68` | 1 | Device/connection-related byte |

The 64-bit timestamp is copied from each raw driver record. `scePadReadState` synthesizes a fallback timestamp using `sceKernelGetProcessTime() - 1000` when it must create a disconnected/default cached record, supporting the interpretation that the timestamp uses the process-time microsecond clock domain.

Use sample timestamps, not only the time at which `scePadRead` returns, when ordering transitions or calculating transport-to-application age.

## API comparison

| API | Steady-state behavior | Recommendation |
|---|---|---|
| `scePadRead` | One driver request; returns 0 to 64 new 120-byte records and a count. | Preferred. Drain a sufficiently large batch and process all returned records. |
| `scePadReadState` | Calls the same core for one record; if none is new, copies the cached latest state. | Use only when intermediate transitions do not matter. |
| `scePadReadExt` | Same batched core with 128-byte output records and an additional capability check. | Use only after the extended structure and required device support are verified. It is not inherently a lower-latency path. |
| `scePadReadStateExt` | Extended equivalent of the one-record/cached-state API. | Same tradeoff as `scePadReadState`. |
| `scePadUiReadState` | Requests a 168-byte internal format and repacks it. | System/UI-specific; extra repacking and no benefit for normal apps. |
| `scePadReadHistory` | Allocates a large temporary buffer and requests a fixed 2400-record history. | Do not use in the frame hot path. |

## Related libraries

### `libSceHidControl`

Its state interfaces target specialized device types. The library does not
expose the general DualSense normalized record queue or the `scePadRead` batch
semantics.

These functions are not a demonstrated low-latency replacement for `scePadRead`. They are candidates only for a separate investigation targeting those specific device classes.

### `libSceBluetoothHid`

This library exposes Bluetooth HID registration, callbacks, report
descriptors, input/feature report requests, and output reports. It is the
transport layer beneath device handling, not the normal application
controller-state API.

Using it directly would require raw HID report parsing and transport lifecycle
handling, and it would exclude or complicate USB-connected controllers. No
observed result demonstrates lower delivery latency than the normalized queue
consumed through `libScePad`.

## Application architecture

### Frame-driven applications

- Keep one owner for each pad handle.
- Call `scePadRead` immediately before simulation consumes input.
- Give it enough capacity to drain any plausible backlog; 64 is the library maximum and costs only 7680 bytes per handle for normal records.
- Process records from first to last for button edges, touch changes, and motion integration.
- Publish the final record as the current state.
- Do not call `scePadReadState` before or after the batch read in the same
  iteration; it adds another serialized driver request and can consume or
  duplicate the latest-state view.

### Dedicated input thread

A dedicated reader can reduce pickup latency below the render-frame interval, but it should be justified by measurement. If used:

- Make it the sole caller for the handle.
- Drain with `scePadRead`, then pass records to the simulation thread through a bounded single-producer/single-consumer queue.
- Preserve the controller timestamps.
- Avoid an unbounded busy loop. Select a yield/wait strategy only after
  measuring empty-read behavior and the controller report interval.

The library uses one global mutex around reads, so parallel polling of multiple handles does not create parallel driver access and can add lock contention.

## Integration note

A current-state abstraction can continue to use `scePadReadState`, but a
low-latency or event-fidelity abstraction should expose a batch method backed by
`scePadRead`. The same 120-byte decoder can process every record in a returned
batch before publishing the final record as current state.

## Practical conclusion

No additional controller library is needed to choose the application API. The
actionable path is runtime measurement and, if desired, a small `scePadRead`
latency probe. Optional extended record fields or specialized controller
classes should be documented separately only when an application needs them.
