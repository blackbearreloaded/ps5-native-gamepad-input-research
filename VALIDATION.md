# Validation

## Evidence levels

Claims in this repository use three labels:

| Label | Meaning |
|---|---|
| Device-tested | Executed successfully in an application on the target device or physically confirmed with a controller, keyboard, or mouse |
| Host-tested | Declarations and example logic pass size, offset, and behavior checks on a PC |
| Pending | Not yet exercised on a device |

An application surviving launch proves initialization, open, and shutdown only.
It does not prove every button, axis, touch, or motion field. Those claims
require physical interaction or telemetry containing non-neutral data.

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

- drains up to 64 records per read;
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
UserService and Pad lifecycle and completed a device run without a loader
error, signal, or crash. That observation confirms startup and shutdown
behavior; it does not independently prove physical controls.

## Working application: native keyboard and mouse

A C++20 test application initialized both libraries and opened indexes for the
initial signed-in user. During a bounded
device run, one wireless keyboard/mouse receiver produced independent streams:

- keyboard index 1 returned timestamped HID usage press/release transitions,
  including simultaneous two-key states and an intercepted record;
- mouse index 0 returned timestamped relative X/Y movement, primary and
  secondary button transitions, vertical wheel values in both directions, and
  horizontal tilt; and
- inactive indexes returned neutral data without destabilizing the process.

A follow-up run removed the receiver for several seconds while all accepted
handles remained open. Both active interfaces reported `connected == 0`.
After reconnection, the same keyboard and mouse handles resumed non-neutral
input without being reopened.

The run confirms physical capture through `libSceKeyboard` and
`libSceMouse` without SDL, raw HID parsing, or pairing code. It does not
establish a universal index assignment for other receivers.

## Current acceptance matrix

| Feature | Host-tested | Device lifecycle | Physical behavior |
|---|---:|---:|---:|
| UserService + standard pad open/close | Yes | Yes | Yes |
| Buttons | Yes | Yes | Yes |
| Left/right sticks | Yes | Yes | Yes |
| Analog L2/R2 | Yes | Yes | Yes |
| Batched short-edge preservation | Yes | Yes | App behavior confirmed |
| Touch contacts/coordinates | Yes | Yes | Pending dedicated fixture |
| Touch-pad click | Yes | Yes | Non-neutral telemetry observed |
| Quaternion/acceleration/gyro | Yes | Yes | Pending dedicated fixture |
| Keyboard init/open and 96-byte layout | Yes | Yes | Yes |
| Keyboard HID usages and multi-key states | Yes | Yes | Yes |
| Keyboard interception | Yes | Yes | Nonzero telemetry observed |
| Mouse init/open and 40-byte layout | Yes | Yes | Yes |
| Mouse relative X/Y | Yes | Yes | Yes |
| Mouse primary/secondary buttons | Yes | Yes | Yes |
| Mouse vertical wheel/horizontal tilt | Yes | Yes | Yes |
| Mouse no-record/neutral-record handling | Yes | Yes | Neutral inactive index observed |
| Keyboard/mouse unplug and reconnect | Yes | Yes | Same handles resumed physical input |
| Multiple simultaneous keyboards or mice | No | No | Pending separate devices |

Remaining device work is listed in [FOLLOW-UP.md](FOLLOW-UP.md).
