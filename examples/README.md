# Examples

All examples include [`../include/ps5_pad.h`](../include/ps5_pad.h) and are
small feature snippets rather than separate packaged applications.

| File | Demonstrates |
|---|---|
| [`01-lifecycle.c`](01-lifecycle.c) | Signed-in user, Pad initialization/open, ownership-aware shutdown |
| [`02-current-state.c`](02-current-state.c) | Cached latest-state polling and correct neutral synthesis |
| [`03-low-latency-batch.c`](03-low-latency-batch.c) | 64-sample drain and chronological press/release edges |
| [`04-full-joystick.c`](04-full-joystick.c) | Both sticks, both analog triggers, full button mask, signed axis conversion |
| [`05-motion-touch.c`](05-motion-touch.c) | Motion configuration and two-contact extraction |
| [`06-feedback.c`](06-feedback.c) | Vibration, light bar, adaptive trigger feedback, safe stop/reset |
| [`07-controller-info.c`](07-controller-info.c) | Connection, device class, reported dead zones, touch resolution |

Run the host check from the repository root:

```sh
make check
```

The examples intentionally omit rendering, logging, packaging, and an infinite
main loop. Copy the relevant function into an existing native app, use its
established error/telemetry path, and call it from that app's lifecycle.

The examples are the public reference implementations. The validation guide
summarizes two larger device-tested applications without linking private source
trees or logs: a batched UI adapter and a full-state streaming adapter.
