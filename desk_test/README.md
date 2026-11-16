# Desk test

Checks the firmware's serial behaviour on a PC, without the screen. It compiles the real firmware files unchanged with plain `g++` and runs them against small stand-ins for the hardware and libraries.

```bash
desk_test/run.sh              # build and run every scenario
desk_test/run.sh screen       # one scenario
VERBOSE=1 desk_test/run.sh    # list passing checks too
```

It needs `g++` (C++17) and nothing else. The exit code is 0 only if every check passes.

## What's real and what's a stand-in

- **Compiled unchanged from `src/`:** `main.cpp`, `Config.cpp`, `FaceRenderer.cpp`, `DisplayInit.cpp`, `LdrSensor.cpp`, `MenuCallbacks.cpp`, `UIManager.cpp`, plus `include/test_GSLC.h` (the GUIslice Builder output, which creates the menu's elements).
- **Stand-ins in `stubs/`:**
  - Arduino `String`, `Serial`, `millis`, `random`, `ledc*`, `analogRead`;
  - `Preferences`, as an in-memory map;
  - TFT_eSPI;
  - the eye library;
  - GUIslice;
  - the touch handler, whose events the test queues.

  They record what the firmware did: lines printed with their time, backlight duty, saved keys, the eyes' last emotion and `blinkNow()` calls, slider positions and button images.
- **GUIslice's slider:** `gslc_ElemXSliderSetPos()` calls the slider callback synchronously when the position changes, as GUIslice's `XSlider.c` does. That is why the firmware needs its echo guards, and the test checks them.
- **Element IDs:** `run.sh` copies the Builder's element enum out of `include/test_GSLC.h` into `build/`, so the IDs always match the firmware's.

## How it runs

Each scenario runs in its own forked process, so each one starts from a fresh boot (`setup()`) with clean firmware state and NVS. A fake clock drives `loop()` in 10 ms steps. Host commands go in as serial lines. Each reply is checked, and no host command may ever be answered with `host_vol`, `host_mute` or `host_mic`.

| Scenario | Covers |
|---|---|
| `boot_defaults`, `boot_saved` | `host_ready` and the boot report; sliders start at their values without echoing or saving |
| `host_audio` | `vol` / `mute` / `mic_mute`: display only, no echo (immediate or delayed), input checks, icons, never saved; panel presses still send `host_mute` / `host_mic` |
| `slider_throttle` | Volume and brightness drags: at most one line per 100 ms, always ending on the final value; `vol:` after a drag doesn't echo |
| `moods` | Moods and unknown moods, `CUSTOM` from `face` / `face_l` / `face_r`, `blink` → `blinkNow()` |
| `cycling` | Order from HAPPY, 5–8 s gaps, the saved mood kept, `host_cycle` |
| `idle_anim` | Only IDLE1–3, 5–15 s gaps, the saved mood kept |
| `brightness` | `host_bright` / `host_auto_bright`, the icon, the saved brightness not drifting under auto, auto-off restoring it, the menu icon, the slider with auto on, `bright:` moving the slider with one `host_bright`, `bright_light` / `bright_dark` 1–100 |
| `screen` | `screen:off` dark with auto-brightness on, no faces, menu closed, `mode:MENU` refused, taps ignored; `screen:on`; a long press while dark turns the screen on without opening the menu |
| `line_buffer` | A stray byte then 300 ms of silence doesn't garble `vol?`; a short pause or a stalled loop doesn't drop a line |
| `host_startup` | The host's start-up sequence and `report` |

## Showing that it bites

Remove the volume slider's echo guard in `src/MenuCallbacks.cpp` (`if (!_settingVolume)` in front of `throttledSend(_volSend, v)`) and run it. Seven checks fail, among them the boot, `vol:` and "no audio request line" checks. Removing the 200 ms line timeout in `serviceConfig()` or the long-press wake in `serviceUI()` fails their checks too.

## What it can't show

What only the real screen can show: drawing, the blink looking immediate, real touch, real UART timing and the backlight actually going dark. Those are on the hand-run checklist with `pio device monitor`.
