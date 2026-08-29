# Validation

## Evidence levels

Claims in this repository use four deliberately narrow labels:

| Label | Meaning |
|---|---|
| Device-tested | Executed successfully in an application on the target device or physically confirmed with a controller |
| Contract-tested | Independently authored declarations pass size, offset, and host-side behavior checks |
| Behavior corroborated | The documented behavior is consistent across compatibility analysis and application telemetry |
| Experimental | A plausible interface exists, but its contract, permissions, or physical behavior remain incomplete |

An application surviving launch proves initialization, open, and shutdown only.
It does not prove every button, axis, touch, motion, or feedback effect. Those
claims require physical interaction or telemetry containing non-neutral data.

## Working application: streaming input

A native streaming application exercised the signed-in-user lifecycle and the
full standard gamepad mapping. Its tested behavior includes:

- opening the initial user's standard controller;
- reading at an approximately 4 ms cadence;
- mapping the D-pad, face buttons, shoulders, stick clicks, Options,
  touch-pad click, analog triggers, and both analog sticks;
- converting stick bytes to signed 16-bit axes and inverting Y;
- sending changed state plus a periodic keepalive;
- sending neutral/removal state before transport shutdown; and
- closing the controller without input or transport errors.

The first implementation used current-state polling. The production version
uses batches of up to 64 records and processes them oldest-first. Device tests
confirmed full joystick operation, non-neutral button data, batches larger
than one record, connection-generation handling, controller-to-mouse mode,
and repeated sessions with zero controller-read or input-send errors.

## Working application: native UI input

A separate native UI application uses the same batched controller path without
initializing an SDL joystick subsystem. Its tested adapter:

- drains up to 64 normal records per read;
- preserves button press and release edges in chronological order;
- maps the D-pad, face buttons, Options, L1, and R1;
- maps the left stick through dead-zone and dominant-axis rules;
- implements a 350 ms initial repeat delay and 110 ms repeat interval;
- treats disconnection and system interception as neutral; and
- stores events in a bounded static queue.

Physical tests covered navigation, paging, search, playback, modal interaction,
and full joystick operation. Host checks cover dead zones, dominant-axis
selection, button edges, neutral synthesis, and axis conversion.

## Additional lifecycle confirmation

An additional native UI build replaced SDL joystick handling with the same
UserService and Pad lifecycle and completed a locked-device observation without
a loader error, signal, or crash. That observation confirms startup and
shutdown behavior; it does not independently prove physical controls.

## Working application: native keyboard and mouse

A C++20 probe loaded the native Keyboard and Mouse sysmodules, initialized both
libraries, and opened indexes for the initial signed-in user. During a bounded
device run, one wireless keyboard/mouse receiver produced independent native
streams:

- keyboard index 1 returned timestamped HID usage press/release transitions,
  including simultaneous two-key states and an intercepted record;
- mouse index 0 returned timestamped relative X/Y movement, primary and
  secondary button transitions, vertical wheel values in both directions, and
  horizontal tilt; and
- inactive indexes returned neutral data without destabilizing the process.

The run confirms physical capture through `libSceKeyboard` and
`libSceMouse` without SDL, raw HID parsing, pairing code, or transport-level
Bluetooth APIs. It does not establish a universal index assignment for other
receivers.

## Publication boundary

The public repository contains only independently authored documentation,
declarations, tests, and examples. Private task links, local source paths,
runtime-log locations, application identifiers, binary fingerprints,
analysis-database references, and internal code addresses are intentionally
excluded. The public evidence is limited to the functional compatibility facts
and aggregate test results needed to reproduce the application behavior.

## Current acceptance matrix

| Feature | Contract/host | Device lifecycle | Physical behavior |
|---|---:|---:|---:|
| UserService + standard pad open/close | Yes | Yes | Yes |
| Buttons | Yes | Yes | Yes |
| Left/right sticks | Yes | Yes | Yes |
| Analog L2/R2 | Yes | Yes | Yes |
| Batched short-edge preservation | Yes | Yes | App behavior confirmed |
| Touch contacts/coordinates | Yes | Yes | Pending dedicated fixture |
| Touch-pad click | Yes | Yes | Non-neutral telemetry observed |
| Quaternion/acceleration/gyro | Yes | Yes | Pending dedicated fixture |
| Controller information | Yes | Not isolated | Pending |
| Vibration | Contract-tested | No | Pending |
| Light bar | Contract-tested | No | Pending |
| Adaptive triggers | Contract-tested | No | Pending |
| Specialized device classes | Partial | No | Pending |
| Keyboard module/init/open and 96-byte layout | Yes | Yes | Yes |
| Keyboard HID usages and multi-key states | Yes | Yes | Yes |
| Keyboard modifiers and LED state | Yes | Yes | Pending dedicated input |
| Keyboard interception | Yes | Yes | Nonzero telemetry observed |
| Mouse module/init/open and 40-byte layout | Yes | Yes | Yes |
| Mouse relative X/Y | Yes | Yes | Yes |
| Mouse primary/secondary buttons | Yes | Yes | Yes |
| Mouse middle/X1/X2 buttons | Yes | Yes | Pending device controls |
| Mouse vertical wheel/horizontal tilt | Yes | Yes | Yes |
| Mouse no-record/neutral-record behavior | Host-tested | Yes | Neutral inactive index observed |

Remaining device work is listed in [FOLLOW-UP.md](FOLLOW-UP.md).
