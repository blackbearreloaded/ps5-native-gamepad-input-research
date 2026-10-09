# Examples

The files are small C++20 feature snippets built against the headers in
[`../include`](../include).

| File | Demonstrates |
|---|---|
| [`01-lifecycle.cpp`](01-lifecycle.cpp) | Signed-in user, RAII Pad initialization/open, ownership-aware shutdown |
| [`02-current-state.cpp`](02-current-state.cpp) | Latest-state polling and correct neutral synthesis |
| [`03-low-latency-batch.cpp`](03-low-latency-batch.cpp) | `std::array`/`std::span` batch drain and chronological edges |
| [`04-full-joystick.cpp`](04-full-joystick.cpp) | Both sticks, both analog triggers, button mask, signed axis conversion |
| [`05-motion-touch.cpp`](05-motion-touch.cpp) | Motion fields and bounded two-contact extraction |
| [`08-keyboard-batch.cpp`](08-keyboard-batch.cpp) | Batched HID-usage press/release edges with disconnect/interception neutralization |
| [`09-mouse-batch.cpp`](09-mouse-batch.cpp) | Batched relative motion/buttons and correct no-record/neutral behavior |

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

The validation guide summarizes two larger device-tested applications: a
batched UI adapter and a full-state streaming adapter.
