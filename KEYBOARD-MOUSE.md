# Native keyboard and mouse input

## Recommended application path

Use `libSceUserService` to resolve the initial signed-in user, then use
`libSceKeyboard` and `libSceMouse` directly. SDL is not required for either
device.

```text
signed-in user
  -> sceKeyboardInit -> sceKeyboardOpen(user, 0, index, nullptr)
  -> sceMouseInit    -> sceMouseOpen(user, 0, index, parameters)
  -> drain both queues immediately before consuming input
  -> process every returned record oldest-first
  -> close both handles during controlled shutdown
```

`sceKeyboardInit` and `sceMouseInit` are process-wide and idempotent in the
analyzed interfaces. There is no application need to pair devices, parse USB
descriptors, or use Bluetooth transport APIs.

## Keyboard contract

### Lifecycle and read functions

The normal application subset is:

- `sceKeyboardInit()`;
- `sceKeyboardOpen(user_id, 0, index, parameters)`;
- `sceKeyboardRead(handle, records, capacity)`;
- `sceKeyboardReadState(handle, &record)`; and
- `sceKeyboardClose(handle)`.

The open-parameter object is eight reserved bytes in this interface. The normal
implementation does not consume it, so pass `nullptr` or a zero-initialized
object. The analyzed implementation maintains twelve logical keyboard slots.

`sceKeyboardRead` accepts capacities from 1 through 16. When more records are
available than requested, it retains the newest requested records and returns
them in chronological order. Use capacity 16 when short press/release fidelity
matters.

`sceKeyboardReadState` returns the latest state and can fall back to the
library's cached state when no new report exists. It is convenient for text
fields that only need held keys. It can miss a complete press/release pair that
occurs between application polls, so batched `sceKeyboardRead` is the preferred
low-latency event path.

### 96-byte keyboard record

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| `0x00` | 8 | `timestamp_us` | Native report timestamp |
| `0x08` | 1 | `intercepted` | Nonzero when the system owns the report |
| `0x09` | 7 | reserved | Zero/reserved |
| `0x10` | 1 | `connected` | Nonzero while the device is available |
| `0x11` | 3 | reserved | Alignment/reserved |
| `0x14` | 4 | `length` | Number of populated key usages; may be 1 for an idle record |
| `0x18` | 4 | `leds` | Num, Caps, and Scroll Lock bits |
| `0x1c` | 4 | `modifiers` | Left/right Control, Shift, Alt, and GUI bits |
| `0x20` | 32 | `keycodes[16]` | 16-bit values containing USB HID keyboard usages |
| `0x40` | 32 | reserved | Zero/reserved |

The key array contains HID usage values, not Unicode characters and not
platform virtual-key values. Ignore usage `0`. The implementation can report
`length == 1` while `keycodes[0] == 0`, including idle or disconnected neutral
states. Iterate the bounded key array and filter zero rather than treating
`length` alone as proof that a key is down.

Modifier bits are:

| Bit | Modifier |
| ---: | --- |
| 0 | Left Control |
| 1 | Left Shift |
| 2 | Left Alt |
| 3 | Left GUI |
| 4 | Right Control |
| 5 | Right Shift |
| 6 | Right Alt |
| 7 | Right GUI |

LED bits 0, 1, and 2 represent Num Lock, Caps Lock, and Scroll Lock.

Text input requires a separate HID-usage-to-layout mapping, dead-key and
composition policy, or a platform text-input service. The exported
key-to-character helper was observed but its complete public contract is not
published here because the parameter and composition-state semantics have not
been device-validated.

## Mouse contract

### Lifecycle and read functions

The normal application subset is:

- `sceMouseInit()`;
- `sceMouseOpen(user_id, 0, index, parameters)`;
- `sceMouseRead(handle, records, capacity)`;
- `sceMouseSetHandType(handle, type)`;
- `sceMouseSetPointerSpeed(handle, speed)`; and
- `sceMouseClose(handle)`.

The analyzed implementation maintains eight logical mouse slots.
`sceMouseRead` accepts capacities from 1 through 64 and retains the newest
requested records if the caller provides less room than the available queue.
Use capacity 64 for a loss-resistant low-latency drain.

### Open behavior

`SceMouseOpenParam` is eight bytes. Bit 0 of the first byte requests merged
behavior on system software that supports it:

```cpp
ps5::mouse::OpenParameters parameters{};
parameters.behavior = ps5::mouse::kBehaviorMerged;
const auto handle = sceMouseOpen(user_id, ps5::mouse::kTypeStandard, 0,
                                 &parameters);
```

Default behavior opens one selected index. Merged behavior opens indexes 0 and
1 as one logical handle. For overlapping raw records, the implementation uses
the longer stream as the base and combines corresponding records by summing
relative axes and wheel values and OR-ing button/interception flags. This is an
index-wise merge, not a global timestamp sort. Open a single index when device
identity or exact per-device chronology matters.

### 40-byte mouse record

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| `0x00` | 8 | `timestamp_us` | Native report timestamp |
| `0x08` | 1 | `connected` | Nonzero while the device is available |
| `0x09` | 3 | reserved | Alignment/reserved |
| `0x0c` | 4 | `buttons` | Five button bits plus interception bit 31 |
| `0x10` | 4 | `x` | Relative horizontal motion |
| `0x14` | 4 | `y` | Relative vertical motion |
| `0x18` | 4 | `wheel` | Relative vertical wheel motion |
| `0x1c` | 4 | `tilt` | Relative horizontal wheel motion |
| `0x20` | 8 | reserved | Zero/reserved |

Button bits 0 through 4 are primary, secondary, middle, X1, and X2. Bit 31
marks an intercepted report. Motion and wheel fields are relative deltas; add
them to application state rather than treating them as absolute coordinates.

### The zero-report rule

When a connected mouse has no new report, `sceMouseRead` can return `0` without
writing a new output record. On zero:

- do not parse the buffer;
- do not replay its old motion or wheel values; and
- do not clear the remembered held-button state.

Only a newly returned record changes buttons. Disconnection or interception
should publish a neutral button state and zero deltas.

Pointer speed accepts values 0 through 8 in the analyzed interface. The normal
read conversion applies that setting before exposing 32-bit relative axes.
Leave the platform default unchanged unless the application has a specific,
device-tested reason to alter it.

## Low-latency loop

Keyboard and mouse reads each enter their library's process-wide serialized
read path and perform one normal HID read per selected device. A practical
frame-driven loop is:

1. drain keyboard records with capacity 16;
2. drain mouse records with capacity 64;
3. process returned records in order;
4. update simulation or UI state; and
5. render.

Read immediately before input consumption to avoid adding an application
frame. One owner should drain each handle. A dedicated input thread is useful
only when the application frame can block longer than the acceptable input
interval; it does not make the native device report cadence faster.

At a 4 ms application poll, the maximum caller storage is 1,536 bytes for
keyboard and 2,560 bytes for mouse. Fixed arrays avoid allocation and keep the
hot path deterministic.

## Examples and probe

- [`examples/08-keyboard-batch.cpp`](examples/08-keyboard-batch.cpp) derives
  chronological key press/release edges from HID usage sets.
- [`examples/09-mouse-batch.cpp`](examples/09-mouse-batch.cpp) preserves held
  buttons across zero-report polls and neutralizes intercepted records.
- [`examples/native-keyboard-mouse-poc`](examples/native-keyboard-mouse-poc)
  is a deployable source overlay that records attached-device activity.

The declarations are in [`include/ps5_keyboard.hpp`](include/ps5_keyboard.hpp)
and [`include/ps5_mouse.hpp`](include/ps5_mouse.hpp). They are independently
authored compatibility material; no proprietary module, SDK header, analysis
database, internal address, or device firmware is included.
