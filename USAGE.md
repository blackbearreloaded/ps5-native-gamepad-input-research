# Usage and lifecycle

## Required libraries

A native application needs imports for:

```text
libSceUserService
libScePad
```

The examples declare functions directly in [`include/ps5_pad.hpp`](include/ps5_pad.hpp).
In a project using generated stubs, add the same symbols to the UserService and
Pad import catalogs. SDL may still provide video, audio-independent timing, or
window events; do not initialize `SDL_INIT_JOYSTICK` when using this path.

## Open the controller

The standard controller belongs to a signed-in user. Opening it as the system
user can succeed yet remain permanently neutral, so obtain the user who started
the application.

```cpp
const auto user_service_result = sceUserServiceInitialize(nullptr);
const bool owns_user_service = user_service_result == 0;

std::int32_t user_id = -1;
if (sceUserServiceGetInitialUser(&user_id) < 0)
    fail();
if (scePadInit() < 0)
    fail();

const auto handle =
    scePadOpen(user_id, ps5::pad::kPortTypeStandard, 0, nullptr);
if (handle < 0)
    fail();
```

The initialization call can report that UserService is already initialized.
That is not automatically fatal: continue to `GetInitialUser`, but call
`sceUserServiceTerminate` at shutdown only when this component's initialize
call returned zero. See [`examples/01-lifecycle.cpp`](examples/01-lifecycle.cpp).

`port_type = 0` is the standard gamepad, and `index = 0` is the first standard
pad for that user. Use one handle owner. If another component already opened
the same tuple, `scePadGetHandle(user, type, index)` can retrieve its handle,
but sharing read ownership requires coordination because reads consume the
same queued state.

## Choose a read model

### Current state

```cpp
ps5::pad::Data state{};
const auto result = scePadReadState(handle, &state);
```

Use this when only the latest state matters and the poll interval is much
shorter than the inputs being observed. The first working streaming adapter
polled at 4 ms, mapped the full gamepad state, sent changes, and sent a
one-second keepalive. The later production client retained the 4 ms cadence but
switched to `scePadRead(..., 64)` after real sessions demonstrated batches as
large as seven records.

`scePadReadState` calls the common read core with capacity one. If no new report
exists, it returns the cached final state. This makes it convenient, but a press
and release that both occur between calls can disappear.

### Batched samples

```cpp
std::array<ps5::pad::Data, ps5::pad::kMaxSamples> samples{};
const auto count = scePadRead(
    handle, samples.data(), static_cast<std::int32_t>(samples.size()));
if (count < 0)
    handle_error(count);

for (const auto& sample :
     std::span{samples}.first(static_cast<std::size_t>(count)))
    process_sample(sample);
```

Use this for frame-driven UIs and games, rhythm-sensitive actions, fighting
games, or any code that must retain short transitions. Capacity must be 1-64.
The function returns 0-64 new records, oldest-first, from one driver request.
When the driver queue contains more than the requested capacity, the library
keeps the newest records that fit.

The maximum batch occupies 7,680 bytes. A fixed stack, static, or per-controller
buffer is sufficient; no heap allocation is needed in the hot path.

## Per-sample rules

Apply these rules before interpreting controls:

```cpp
const bool neutral = !ps5::pad::is_usable(sample);
```

If neutral:

- buttons are released;
- sticks are centered at 128, not zero;
- analog triggers are zero;
- touch count is zero;
- motion should not drive application behavior.

A zero-filled raw structure is not neutral because stick byte zero is maximum
negative deflection. Initialize synthesized state with both sticks at 128 and
orientation `w = 1.0f`.

## Frame-driven loop

The recommended order is:

```text
drain operating-system/window events
scePadRead(..., 64)
process every button/touch transition
publish final sample as current analog/motion state
run simulation or UI update
render and present
```

Reading after simulation delays every input by one application frame. Reading
once near consumption avoids that delay and avoids a second serialized driver
request.

Do not call `scePadReadState` before or after a batch read on the same handle in
the same iteration. Both reach the same globally locked read core.

## High-rate streaming loop

For remote-play or input-forwarding workloads, the initial proven design was:

```text
poll every 4 ms
read current state
normalize axes and map buttons
send only changed state
send unchanged state once per second as a keepalive
```

The production streaming integration now uses a maximum batch read at that same
cadence. Process every button edge, then retain the last sample's analog values.
On a zero-count read, do not create edges; only maintain required protocol
keepalives or time-based behavior from the last valid state. A dedicated reader
thread is justified only when measurements show that the main loop's cadence
dominates sample age.

See [Production integration findings](PRODUCTION-INTEGRATION.md) for observed
batch sizes, protocol lifecycle, controller-to-mouse mode, and the distinction
between Pad acquisition latency and displayed-frame latency.

## Multiple users and controllers

Open one standard handle per signed-in user and poll each from one owner. The
library uses a process-wide mutex around driver reads, so simultaneous polling
threads do not create parallel driver access. Poll handles serially unless
measurement shows a need for a different design. Application policy and
platform limits still apply to the number of local controllers.

## Shutdown

Stop outputs before closing:

```cpp
const ps5::pad::Vibration stop{};
scePadSetVibration(handle, &stop);
scePadResetLightBar(handle);
scePadClose(handle);

if (owns_user_service)
    sceUserServiceTerminate();
```

If a protocol-facing adapter announced a controller to a remote host, send its
neutral/removal event before closing the handle and before tearing down the
transport.

## Error handling

All functions return a negative error code on failure. `scePadOpen` returns a
non-negative handle on success; `scePadRead` returns a non-negative sample
count. Preserve the raw code in diagnostics, but design gameplay to tolerate
temporary disconnection by publishing neutral state rather than retaining a
stuck button or stick.

Do not log through unverified `printf`/`puts` imports in early homebrew startup.
The working native projects use their established telemetry or UI status paths.
