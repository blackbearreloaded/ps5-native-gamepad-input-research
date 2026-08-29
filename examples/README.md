# Examples

The portable files are small C++20 feature snippets built against the
independently authored headers in [`../include`](../include). The native probe
subdirectory is a source overlay for a packaged application.

| File | Demonstrates |
|---|---|
| [`01-lifecycle.cpp`](01-lifecycle.cpp) | Signed-in user, RAII Pad initialization/open, ownership-aware shutdown |
| [`02-current-state.cpp`](02-current-state.cpp) | Cached latest-state polling and correct neutral synthesis |
| [`03-low-latency-batch.cpp`](03-low-latency-batch.cpp) | `std::array`/`std::span` batch drain and chronological edges |
| [`04-full-joystick.cpp`](04-full-joystick.cpp) | Both sticks, both analog triggers, full button mask, signed axis conversion |
| [`05-motion-touch.cpp`](05-motion-touch.cpp) | Motion configuration and bounded two-contact extraction |
| [`06-feedback.cpp`](06-feedback.cpp) | Vibration, light bar, adaptive trigger feedback, safe stop/reset |
| [`07-controller-info.cpp`](07-controller-info.cpp) | Connection, device class, reported dead zones, touch resolution |
| [`08-keyboard-batch.cpp`](08-keyboard-batch.cpp) | Batched HID-usage press/release edges with disconnect/interception neutralization |
| [`09-mouse-batch.cpp`](09-mouse-batch.cpp) | Batched relative motion/buttons and correct no-record/neutral behavior |
| [`native-keyboard-mouse-poc`](native-keyboard-mouse-poc) | Attached keyboard/mouse notification and `/download0` evidence probe |

Run the host check from the repository root:

```sh
make check
```

The examples use fixed-size standard containers, value initialization,
namespaces, references, `constexpr` helpers, and RAII for owned resources.
They require no exceptions, RTTI, heap allocation, or framework layer.

They intentionally omit rendering, logging, packaging, and an infinite main
loop. Copy the relevant type or function into an existing native app, use its
established error/telemetry path, and call it from that app's lifecycle.

The examples are the public reference implementations. The validation guide
summarizes two larger device-tested applications without linking private source
trees or logs: a batched UI adapter and a full-state streaming adapter.
