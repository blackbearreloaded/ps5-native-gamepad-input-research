# PS5 Native Controller Input

[![Examples](https://github.com/blackbearreloaded/ps5-native-input/actions/workflows/examples.yml/badge.svg)](https://github.com/blackbearreloaded/ps5-native-input/actions/workflows/examples.yml)
[![Input API](https://img.shields.io/badge/input-libScePad-003791.svg)](LIBRARIES.md)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)

Independently documented and device-tested guidance for native, low-latency
PlayStation 5 controller input. The repository covers controller
lifecycle, batched and current-state reads, every verified DualSense field,
motion, touch, feedback, adaptive triggers, and the responsibilities of the
underlying PS5 libraries.

The normal application path is deliberately small: use `libSceUserService` to
resolve the signed-in player and `libScePad` for input and output. The working
applications do not require SDL joystick support, raw Bluetooth HID reports,
controller pairing code, or a custom input thread.

## Project status

| Area | Status |
| --- | --- |
| Signed-in user and pad lifecycle | Device-tested |
| Digital buttons | Device-tested in working applications |
| Left and right sticks | Device-tested in working applications |
| Analog L2 and R2 | Device-tested in working applications |
| Batched low-latency reads | Device-tested in UI and streaming applications |
| Current-state polling | Device-tested at approximately 243–250 Hz |
| Sample timestamp and connection generation | Contract-tested and behavior corroborated |
| Two-contact touch data | Contract-tested; dedicated physical test pending |
| Orientation, acceleration, and angular velocity | Contract-tested; dedicated physical test pending |
| Controller information and reported dead zones | Contract-tested and behavior corroborated |
| Vibration and light bar | Contract-tested; repository device test pending |
| Adaptive trigger effects and state | Contract-tested; repository device test pending |
| Specialized device classes | Partial compatibility evidence only |
| CI | Builds strict C11 examples and runs host-side contract checks |

Platform interfaces can change. Re-run the layout checks and device tests
before assuming compatibility with another system-software version.

## Main findings

| Finding | Result |
| --- | --- |
| Recommended libraries | `libSceUserService` + `libScePad` |
| Lowest-risk low-latency API | `scePadRead(handle, samples, 64)` |
| Driver interaction | One platform-driver request per batch read |
| Returned order | Oldest retained sample first; newest sample last |
| Maximum normal batch | 64 records, 7,680 bytes total |
| Normal record size | 120 bytes |
| Current-state API | `scePadReadState`, with cached-state fallback |
| Short button transitions | Preserved by processing every batched record |
| Observed production batch | Up to 7 records despite a 4 ms application poll |
| USB versus Bluetooth | Hidden behind the same Pad API |
| `libSceHidControl` | Specialized Jedi/Spark service controls, not the normal gamepad queue |
| `libSceBluetoothHid` | Raw Bluetooth transport, not a faster application input path |

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
simulation adds an avoidable application frame of latency. Do not surround the
batch read with `scePadReadState` calls on the same handle; both enter the same
globally locked read core.

## Capability matrix

| Capability | Native representation | Notes |
| --- | --- | --- |
| Create, Options, D-pad, face buttons | `ScePadData.buttons` | Verified button mask |
| L1/R1, L2/R2, L3/R3 | Digital mask | L2/R2 also provide analog travel |
| Touch-pad click | Digital mask bit `0x00100000` | Separate from touch contacts |
| System interception | Mask bit `0x80000000` | Discard the sample and publish neutral state |
| Left/right sticks | Four unsigned bytes | Range 0–255, center 128 |
| Analog triggers | Two unsigned bytes | Range 0–255 |
| Touch pad | Up to two contacts | X/Y, stable contact ID, controller-specific resolution |
| Motion | Quaternion + acceleration + angular velocity | Timestamp every sample |
| Connection | 32-bit state + generation byte | Clear cached edges/calibration when generation changes |
| Vibration | Two unsigned levels | Low- and high-frequency components |
| Light bar | RGB parameter | Reset restores platform behavior |
| Adaptive triggers | Off, feedback, weapon, vibration | Separate L2/R2 commands and state values |
| PS button | System-owned | Not exposed as a normal application button |
| Mute button and controller audio | Separate services | Not verified in the normal 120-byte Pad record |

See [Controller data format](CONTROLLER-DATA.md) for the exact offsets, button
values, coordinate handling, timestamps, and connection rules.

## Quick start

The examples are portable C11 contract snippets. Host checks require no vendor
SDK, firmware files, proprietary headers, or console toolchain.

```sh
git clone git@github.com:blackbearreloaded/ps5-native-input.git
cd ps5-native-input
make check
```

`make check` verifies the documented structure sizes and offsets, exercises
button-edge and neutral-state behavior, checks axis conversion, and compiles
every example with `-Wall -Wextra -Werror -pedantic`.

For a native PS5 application, import `libSceUserService` and `libScePad`, then
use the declarations in [`include/ps5_pad.h`](include/ps5_pad.h):

```c
int32_t user_id;
sceUserServiceInitialize(NULL);
sceUserServiceGetInitialUser(&user_id);
scePadInit();

int32_t handle = scePadOpen(user_id, PS5_PAD_PORT_TYPE_STANDARD, 0, NULL);

ps5_pad_data_t samples[PS5_PAD_MAX_SAMPLES];
int32_t count = scePadRead(handle, samples, PS5_PAD_MAX_SAMPLES);
for (int32_t i = 0; i < count; ++i)
    process_sample(&samples[i]);
```

Production code must check every return value, neutralize disconnected or
intercepted records, stop active output effects, and close the handle during
shutdown. [Usage and lifecycle](USAGE.md) provides the complete sequence.

## Implementation guidance

1. Open a standard pad for the real signed-in user, not the system user.
2. Use `scePadRead(..., 64)` when press/release fidelity matters.
3. Process every returned record in order and retain the final record as the
   current analog, touch, and motion state.
4. Treat disconnection and system interception as a neutral controller; raw
   zero-filled stick fields are not neutral because their center is 128.
5. Use sample timestamps for ordering and motion integration, not render time.
6. Read once near input consumption and keep one owner per pad handle.
7. Use `libScePad` feedback calls instead of constructing raw Bluetooth output
   reports.
8. Validate touch, motion, vibration, lighting, and trigger effects on the
   target controller and firmware before advertising those features.

## Documentation

| Document | Purpose |
| --- | --- |
| [Usage and lifecycle](USAGE.md) | Imports, signed-in user, open/read/close, polling models, multi-user handling, and errors |
| [Controller data format](CONTROLLER-DATA.md) | Exact 120-byte layout, buttons, axes, touch, motion, timing, and connection state |
| [Controller features](FEATURES.md) | Information, dead zones, motion configuration, vibration, light bar, and adaptive triggers |
| [Library responsibilities](LIBRARIES.md) | UserService and Pad versus HidControl and BluetoothHid |
| [Low-latency investigation](INVESTIGATION.md) | Queue behavior, driver path, API comparison, and recommended architecture |
| [Static compatibility evidence](STATIC-EVIDENCE.md) | Functional read behavior, independently documented layouts, and library conclusions |
| [Validation](VALIDATION.md) | Working applications, aggregate device evidence, publication boundary, and acceptance matrix |
| [Production integration](PRODUCTION-INTEGRATION.md) | Batched streaming telemetry, protocol lifecycle, local chords, mouse mode, and latency diagnosis |
| [Follow-up experiments](FOLLOW-UP.md) | Remaining latency measurements and optional ABI investigations |
| [Examples](examples/README.md) | Feature-focused native C examples |

## Examples

| Example | Demonstrates |
| --- | --- |
| [`01-lifecycle.c`](examples/01-lifecycle.c) | Signed-in user, ownership-aware initialization, open, and shutdown |
| [`02-current-state.c`](examples/02-current-state.c) | Cached latest-state polling and correct neutral synthesis |
| [`03-low-latency-batch.c`](examples/03-low-latency-batch.c) | 64-record drain with chronological press/release edges |
| [`04-full-joystick.c`](examples/04-full-joystick.c) | Both sticks, analog triggers, complete button mask, and signed axes |
| [`05-motion-touch.c`](examples/05-motion-touch.c) | Motion configuration and two-contact extraction |
| [`06-feedback.c`](examples/06-feedback.c) | Vibration, light bar, adaptive-trigger feedback, and safe reset |
| [`07-controller-info.c`](examples/07-controller-info.c) | Connection, device class, dead zones, and touch resolution |

The validation guide also summarizes two complete integrations: a native
controller-forwarding path and an SDL-free UI adapter. Both were physically
exercised with a DualSense; private application paths and logs are intentionally
not part of this repository.

## Repository layout

```text
README.md                       Research overview and application guidance
USAGE.md                        Initialization, polling, shutdown, and errors
CONTROLLER-DATA.md              Exact input-record contract
FEATURES.md                     Motion, touch, feedback, and controller information
LIBRARIES.md                    Native library selection and scope
INVESTIGATION.md                Low-latency read-path analysis
STATIC-EVIDENCE.md              Functional compatibility evidence
VALIDATION.md                   Device evidence and publication boundary
PRODUCTION-INTEGRATION.md       Production batching, multiplexing, and telemetry lessons
FOLLOW-UP.md                    Remaining optional experiments
include/ps5_pad.h               Independently authored compatibility declarations
examples/                       Small feature-focused C examples
tests/layout_check.c            ABI size and offset regression
tests/logic_check.c             Edge, neutral-state, and axis regression
.github/workflows/examples.yml  Host-side CI validation
```

## Methodology

The investigation combined independently authored compatibility declarations,
host-side contract tests, functional analysis of the application-facing
interfaces, and runtime behavior from working applications using physical
controllers. Proprietary files, private task data, binary fingerprints,
analysis databases, and internal addresses are not published.

Evidence labels are intentionally narrow:

- **Device-tested:** the path executed successfully on a PS5 or was physically
  confirmed with a controller.
- **Contract-tested:** independently authored sizes, offsets, values, or
  prototypes passed layout and host-side checks.
- **Behavior corroborated:** functional conclusions agree with application
  telemetry and compatibility analysis.
- **Experimental:** a plausible interface exists, but physical behavior,
  privileges, or some structure fields remain incomplete.

## Scope

This repository publishes independently authored documentation and small
examples. It does not contain vendor SDK files, firmware or system modules,
retail binaries, decrypted content, signing material, credentials,
cryptographic keys, analysis databases, controller firmware, copied vendor
source, or proprietary headers. It does not provide an exploit or access-control
bypass, pair controllers, update controller firmware, or replace platform
device policy.

The PS button, microphone mute control, speaker/microphone audio, headset
routing, raw Bluetooth registration, and privileged system interception are
outside the verified normal application-input contract.

See [NOTICE.md](NOTICE.md) for the legal-use boundary, third-party rights, and
trademark attribution.

## License and attribution

Repository-authored documentation and examples are licensed under GPL-3.0.
See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).

PlayStation, PS5, and DualSense are trademarks of Sony Interactive
Entertainment. This independent project is not affiliated with or endorsed by
Sony Interactive Entertainment.
