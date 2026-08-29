# Native keyboard and mouse probe

This C++20 discovery probe attempts indexed keyboard and mouse opens for the
initial signed-in user, drains every successfully opened native queue every
4 ms, shows one notification when each device produces physical input, and
appends bounded records to:

```text
/download0/ps5-input-probe.log
```

The source is intended as a small overlay for a known-working native app
boilerplate. Add this repository's `include/` directory to the compiler include
path and import `libSceUserService` plus the kernel functions used by
`src/main.cpp`.

At startup, the probe calls `sceSysmoduleLoadModule` with the independently
documented Keyboard and Mouse module IDs, initializes both libraries, and uses
direct native imports. Symbol lookup is not part of the input loop.

The broad index scan is diagnostic behavior. It logs each open result and the
source index of every connected record. Once an application's device index is
known, production code should open and poll only that index. A tested wireless
combination receiver exposed its keyboard on index 1 and mouse on index 0,
showing that interfaces sharing one USB receiver need not share an index.

`link-stubs/mouse_link_stub.cpp` is available when a community toolchain lacks
the Mouse import stub. Build it as a temporary shared provider named
`libSceMouse.prx`, pass it to the native module-linking step as a system-module
stub, and do not package its fallback bodies in the application.

The probe deliberately avoids SDL, pairing, raw HID transport, and privileged
device-control interfaces. It records USB HID usage values rather than mapping
them to text; text composition and keyboard-layout handling are separate tasks.
