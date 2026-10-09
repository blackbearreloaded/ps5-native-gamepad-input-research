# Native keyboard and mouse input

## Recommended application path

Use `libSceUserService` to resolve the initial signed-in user, then use
`libSceKeyboard` and `libSceMouse` directly. SDL is not required for either
device.

```text
signed-in user
  -> sceKeyboardInit
  -> sceKeyboardOpen(user, 0, index, parameters)
  -> sceMouseInit
  -> sceMouseOpen(user, 0, index, parameters)
  -> drain both queues immediately before consuming input
  -> process every returned record oldest-first
  -> close both handles during controlled shutdown
```

The tested application loaded each library before its first call into it; how
to do that depends on the toolchain in use. It did not pair devices, parse USB
descriptors, or use Bluetooth transport APIs.

### Finding the physical index

On the tested setup, indexes 0 and 1 opened successfully in both libraries,
while higher indexes were rejected. A single wireless keyboard/mouse USB
receiver then produced:

| Interface | Active index | Physically observed data |
| --- | ---: | --- |
| Keyboard | 1 | HID usage press/release records, two-key states, timestamps, and interception |
| Mouse | 0 | Relative X/Y, primary/secondary buttons, vertical wheel, horizontal tilt, and timestamps |

This assignment is evidence from one receiver, not a universal index rule. A
robust application should open every accepted index, retain every successful
handle, and track `connected` independently for each one. Keeping inactive
handles allows a later attachment, reconnection on a different index, or
multiple same-class devices to be discovered without restarting the input
system.

### Hot-plug and reconnect validation

The retained-handle strategy was physically tested by removing the active
wireless receiver for several seconds and reconnecting it. The already-open
keyboard index 1 and mouse index 0 handles each returned a neutral
`connected == 0` record. After the receiver returned, those same handles
resumed timestamped keyboard and mouse input without another initialization or
open call.

This validates removal and reconnection for the tested combination receiver.
Simultaneous independent keyboards or mice still need a separate multi-device
test.

## Keyboard

### Lifecycle and read functions

- `sceKeyboardInit()`;
- `sceKeyboardOpen(user_id, 0, index, parameters)`;
- `sceKeyboardRead(handle, records, capacity)`; and
- `sceKeyboardClose(handle)`.

The tested application passed eight zero bytes as open parameters and read with a
capacity of 16 records. Process every returned record in order when short
press/release fidelity matters.

### 96-byte keyboard record

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| `0x00` | 8 | `timestamp_us` | Report timestamp |
| `0x08` | 1 | `system_owned` | Nonzero when the system owns the report |
| `0x09` | 7 | padding | |
| `0x10` | 1 | `connected` | Nonzero while the device is available |
| `0x11` | 3 | padding | |
| `0x14` | 4 | `key_count` | Key count; may be 1 for an idle record |
| `0x18` | 8 | `unverified0` | Not verified on a device |
| `0x20` | 32 | `usages[16]` | 16-bit values containing USB HID keyboard usages |
| `0x40` | 32 | `unverified1` | Not verified on a device |

The key array contains HID usage values, not Unicode characters. Ignore usage
`0`. Iterate the bounded key array and filter zero rather than treating
`key_count` alone as proof that a key is down.

Modifier keys and lock-LED state have not been verified on a device and are
not declared.

Text input requires a separate HID-usage-to-layout mapping and dead-key and
composition policy in the application.

## Mouse

### Lifecycle and read functions

- `sceMouseInit()`;
- `sceMouseOpen(user_id, 0, index, parameters)`;
- `sceMouseRead(handle, records, capacity)`; and
- `sceMouseClose(handle)`.

The tested application passed eight zero bytes as open parameters, opened one index
per handle, and read with a capacity of 64 records.

### 40-byte mouse record

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| `0x00` | 8 | `timestamp_us` | Report timestamp |
| `0x08` | 1 | `connected` | Nonzero while the device is available |
| `0x09` | 3 | padding | |
| `0x0c` | 4 | `buttons` | Bit 0 primary, bit 1 secondary |
| `0x10` | 4 | `x` | Relative horizontal motion |
| `0x14` | 4 | `y` | Relative vertical motion |
| `0x18` | 4 | `wheel` | Relative vertical wheel motion |
| `0x1c` | 4 | `wheel_horizontal` | Relative horizontal wheel motion |
| `0x20` | 8 | `unverified` | Not verified on a device |

Motion and wheel fields are relative deltas; add them to application state
rather than treating them as absolute coordinates. Other button bits have not
been verified on a device and are not declared.

### No-record and neutral-record rules

Handle these two outcomes of `sceMouseRead` differently:

- a return value of `0` supplies no output record; and
- a positive count can include a zero-filled neutral record for an inactive or
  disconnected index.

On a zero return:

- do not parse the buffer;
- do not replay its old motion or wheel values; and
- do not clear the remembered held-button state.

Process every record included in a positive return count. A returned neutral
or disconnected record should publish neutral buttons and zero deltas. In the
device test, an inactive mouse index repeatedly returned one zero-filled
record; filtering those records by `connected` exposed the active index's
physical reports.

## Low-latency loop

A practical frame-driven loop is:

1. drain keyboard records with capacity 16;
2. drain mouse records with capacity 64;
3. process returned records in order;
4. update simulation or UI state; and
5. render.

Read immediately before input consumption to avoid adding an application
frame. One owner should drain each handle. A dedicated input thread is useful
only when the application frame can block longer than the acceptable input
interval.

For hot-plug support, continue polling every successfully opened index and
treat `connected` transitions as device arrival/removal. The test application
polls all accepted handles at the same 4 ms cadence; optimize inactive polling
only after measuring a real cost.

Caller storage is 1,536 bytes for a 16-record keyboard batch and 2,560 bytes
for a 64-record mouse batch. Fixed arrays avoid allocation.

## Examples

- [`examples/08-keyboard-batch.cpp`](examples/08-keyboard-batch.cpp) derives
  chronological key press/release edges from HID usage sets.
- [`examples/09-mouse-batch.cpp`](examples/09-mouse-batch.cpp) preserves held
  buttons when no record is returned and neutralizes disconnected records.

The declarations are in [`include/ps5_keyboard.hpp`](include/ps5_keyboard.hpp)
and [`include/ps5_mouse.hpp`](include/ps5_mouse.hpp).
