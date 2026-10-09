# PS5 Native Gamepad, Keyboard and Mouse Input Research

[![Examples](https://github.com/blackbearreloaded/ps5-native-gamepad-input-research/actions/workflows/examples.yml/badge.svg)](https://github.com/blackbearreloaded/ps5-native-gamepad-input-research/actions/workflows/examples.yml)
[![Input APIs](https://img.shields.io/badge/input-Pad%20%7C%20Keyboard%20%7C%20Mouse-555555.svg)](USAGE.md)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](LICENSE)

Notes and small C++20 examples for native, low-latency input on a PS5:
gamepad, USB keyboard, and USB mouse lifecycle and batched reads, with the
record fields that the device tests in this repository exercised.

The application path is small: use `libSceUserService` to resolve the
signed-in player, then `libScePad`, `libSceKeyboard`, or `libSceMouse` for the
selected device. The tested applications used no SDL input subsystem, raw HID
report parser, or controller pairing code.

## Project status

| Area | Status |
| --- | --- |
| Signed-in user and pad lifecycle | Device-tested |
| Digital buttons | Device-tested in working applications |
| Left and right sticks | Device-tested in working applications |
| Analog L2 and R2 | Device-tested in working applications |
| Batched low-latency reads | Device-tested in UI and streaming applications |
| Current-state polling | Device-tested at approximately 243–250 Hz |
| Sample timestamp and connection generation | Used by the device-tested streaming client |
| Two-contact touch data | Host-tested layout; dedicated physical test pending |
| Orientation, acceleration, and angular velocity | Host-tested layout; dedicated physical test pending |
| Keyboard queue and 96-byte record | Device-tested with timestamped HID key transitions |
| Mouse queue and 40-byte record | Device-tested with motion, clicks, wheel, and tilt |
| Keyboard/mouse hot-plug | Device-tested with retained handles across receiver removal and reconnection |
| CI | Builds strict C++20 examples and runs host-side checks |

Platform interfaces can change. Re-run the layout checks and device tests
before assuming compatibility with another system-software version.

## Main findings

| Finding | Result |
| --- | --- |
| Libraries used | `libSceUserService` plus `libScePad`, `libSceKeyboard`, or `libSceMouse` |
| Low-latency gamepad read | `scePadRead(handle, samples, 64)` |
| Returned order | Oldest sample first; newest sample last |
| Batch used by the tested applications | 64 records, 7,680 bytes total |
| Gamepad record size | 120 bytes |
| Current-state read | `scePadReadState` |
| Short button transitions | Preserved by processing every batched record |
| Observed production batch | Up to 7 records despite a 4 ms application poll |
| Keyboard path | `sceKeyboardRead(handle, samples, 16)` with USB HID usages |
| Mouse path | `sceMouseRead(handle, samples, 64)` with relative axes and wheels |
| Physical index discovery | Open every accepted index and keep successful handles for hot-plug |
| Inactive index | A neutral record can be returned; process exactly the returned count |
| Hot-plug behavior | Existing handles reported removal and resumed input after reconnection; no reopen was needed |

## Recommended input path

```text
Signed-in application user
  -> scePadInit
  -> scePadOpen(user, standard port, index 0)
  -> scePadRead(handle, records, 64)
  -> neutralize disconnected or system-intercepted records
  -> process every button and touch transition oldest-first
  -> publish the final record as current analog and motion state
  -> simulation or UI update
  -> render
```

Call the read immediately before the application consumes input. Reading after
simulation adds an avoidable application frame of latency. Use one read call
per handle and iteration.

## Capability matrix

| Capability | Representation | Notes |
| --- | --- | --- |
| Create, Options, D-pad, face buttons | `ps5::pad::Data::buttons` | Device-tested button mask |
| L1/R1, L2/R2, L3/R3 | Digital mask | L2/R2 also provide analog travel |
| Touch-pad click | Digital mask bit `0x00100000` | Separate from touch contacts |
| System interception | Mask bit `0x80000000` | Discard the sample and publish neutral state |
| Left/right sticks | Four unsigned bytes | Range 0–255, center 128 |
| Analog triggers | Two unsigned bytes | Range 0–255 |
| Touch pad | Up to two contacts | X/Y and contact ID; physical test pending |
| Motion | Quaternion + acceleration + angular velocity | Physical test pending |
| Connection | 32-bit state + generation byte | Clear cached edges when the generation changes |
| PS button | System-owned | Not seen as an application button |

See [Controller data format](CONTROLLER-DATA.md) for offsets, button values,
coordinate handling, timestamps, and connection rules.

Physical keyboards and mice use separate queues. See
[Native keyboard and mouse input](KEYBOARD-MOUSE.md) for their layouts,
lifecycle, idle rules, and C++20 examples.

## Quick start

The examples are portable C++20 snippets. The host checks need only a C++20
compiler and `make`.

```sh
git clone git@github.com:blackbearreloaded/ps5-native-gamepad-input-research.git
cd ps5-native-gamepad-input-research
make check
```

`make check` verifies the gamepad, keyboard, and mouse structure sizes and
offsets; exercises edge and neutral-state behavior; checks axis conversion and
mouse no-record handling; and compiles every portable example with strict
C++20 warnings.

For a native PS5 application, import `libSceUserService` and `libScePad`, then
use the declarations in [`include/ps5_pad.hpp`](include/ps5_pad.hpp):

```cpp
#include "ps5_pad.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

std::int32_t user_id{};
sceUserServiceInitialize(nullptr);
sceUserServiceGetInitialUser(&user_id);
scePadInit();

const auto handle =
    scePadOpen(user_id, ps5::pad::kPortStandard, 0, nullptr);

std::array<ps5::pad::Data, ps5::pad::kBatchSamples> samples{};
const auto count = scePadRead(
    handle, samples.data(), static_cast<std::int32_t>(samples.size()));
if (count < 0)
    handle_error(count);

for (const auto& sample :
     std::span{samples}.first(static_cast<std::size_t>(count)))
    process_sample(sample);
```

Production code must check every return value, neutralize disconnected or
intercepted records, and close the handle during shutdown.
[Usage and lifecycle](USAGE.md) provides the complete sequence.

## Implementation guidance

1. Open a standard pad for the real signed-in user, not the system user.
2. Use `scePadRead(..., 64)` when press/release fidelity matters.
3. Process every returned record in order and retain the final record as the
   current analog, touch, and motion state.
4. Treat disconnection and system interception as a neutral controller; raw
   zero-filled stick fields are not neutral because their center is 128.
5. Use sample timestamps for ordering, not render time.
6. Read once near input consumption and keep one owner per pad handle.
7. Validate touch and motion on the target controller and system software
   before advertising those features.

## Documentation

| Document | Purpose |
| --- | --- |
| [Usage and lifecycle](USAGE.md) | Imports, signed-in user, open/read/close, polling models, and errors |
| [Controller data format](CONTROLLER-DATA.md) | 120-byte layout, buttons, axes, touch, motion, timing, and connection state |
| [Native keyboard and mouse input](KEYBOARD-MOUSE.md) | Keyboard/mouse lifecycle, records, batching, HID usages, relative axes, and idle rules |
| [Validation](VALIDATION.md) | Working applications, aggregate device evidence, and acceptance matrix |
| [Production integration](PRODUCTION-INTEGRATION.md) | Batched streaming telemetry, protocol lifecycle, local chords, mouse mode, and latency diagnosis |
| [Follow-up experiments](FOLLOW-UP.md) | Remaining latency measurements and device tests |
| [Examples](examples/README.md) | Feature-focused C++20 examples |

## Examples

| Example | Demonstrates |
| --- | --- |
| [`01-lifecycle.cpp`](examples/01-lifecycle.cpp) | Signed-in user, ownership-aware initialization, RAII open, and shutdown |
| [`02-current-state.cpp`](examples/02-current-state.cpp) | Latest-state polling and correct neutral synthesis |
| [`03-low-latency-batch.cpp`](examples/03-low-latency-batch.cpp) | 64-record drain with chronological press/release edges |
| [`04-full-joystick.cpp`](examples/04-full-joystick.cpp) | Both sticks, analog triggers, button mask, and signed axes |
| [`05-motion-touch.cpp`](examples/05-motion-touch.cpp) | Motion fields and two-contact extraction |
| [`08-keyboard-batch.cpp`](examples/08-keyboard-batch.cpp) | Sixteen-record keyboard drain and chronological HID-usage edges |
| [`09-mouse-batch.cpp`](examples/09-mouse-batch.cpp) | Sixty-four-record relative mouse drain and correct no-record/neutral handling |

The validation guide also summarizes two complete integrations: a native
controller-forwarding path and an SDL-free UI adapter. Both were physically
exercised with a DualSense.

## Repository layout

```text
README.md                       Overview and application guidance
USAGE.md                        Initialization, polling, shutdown, and errors
CONTROLLER-DATA.md              Gamepad record layout
KEYBOARD-MOUSE.md               Keyboard and mouse records and lifecycle
VALIDATION.md                   Device evidence and acceptance matrix
PRODUCTION-INTEGRATION.md       Production batching, multiplexing, and telemetry lessons
FOLLOW-UP.md                    Remaining experiments
include/ps5_*.hpp               C++20 declarations used by the examples
examples/                       Small feature-focused C++20 examples
tests/layout_check.cpp          Size and offset regression
tests/logic_check.cpp           Edge, neutral-state, and axis regression
tests/keyboard_mouse_check.cpp  Keyboard/mouse queue handling regression
.github/workflows/examples.yml  Host-side CI
```

## Evidence labels

- **Device-tested:** the path executed on a PS5 or was physically confirmed
  with a controller, keyboard, or mouse.
- **Host-tested:** sizes, offsets, or example logic passed `make check` on a
  PC; this says nothing about console behavior.
- **Pending:** not yet exercised on a device.

The PS button, microphone mute control, controller audio, vibration, light
bar, and adaptive triggers are outside what this repository covers.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for third-party rights
and trademark attribution.

## License and notices

Copyright © 2026 BlackBearReloaded. Licensed under GPL-3.0-or-later; see [LICENSE](LICENSE).

This is a personal hobby project. It is not affiliated with, endorsed by or sponsored by Sony Interactive Entertainment or any other company named here. Product names are trademarks of their owners and are used only to describe compatibility. DualSense is a trademark of Sony Interactive Entertainment Inc.

Provided as is, without warranty. Nothing here is legal advice.
