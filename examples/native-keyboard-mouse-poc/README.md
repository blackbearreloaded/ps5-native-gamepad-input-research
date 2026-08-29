# Native keyboard and mouse probe

This C++20 probe opens keyboard index 0 and mouse index 0 for the initial
signed-in user, drains both native queues every 4 ms, shows one notification
when each device produces physical input, and appends bounded records to:

```text
/download0/ps5-input-probe.log
```

The source is intended as a small overlay for a known-working native app
boilerplate. Add this repository's `include/` directory to the compiler include
path and import `libSceUserService`, `libSceKeyboard`, `libSceMouse`, and the
kernel functions used by `src/main.cpp`.

Some community payload SDK snapshots provide a keyboard import stub but omit a
mouse stub. `link-stubs/mouse_link_stub.cpp` is a clean-room link declaration
for that case. Build it as a temporary shared provider named
`libSceMouse.prx`, pass it to the native module-linking step as a system-module
stub, and do not package its fallback bodies in the application.

The probe deliberately avoids SDL, pairing, raw HID transport, and privileged
device-control interfaces. It records USB HID usage values rather than mapping
them to text; text composition and keyboard-layout handling are separate tasks.
