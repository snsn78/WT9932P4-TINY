# AI Agent Instructions

This repository contains an ESP32-P4 / LVGL 9 touch-screen virtual gamepad project.
Read this file before editing anything.

## Project Goal

Make the WT9932P4-TINY / ESP32-P4 board display a touch-screen gamepad UI on a 7-inch 1024x600 MIPI-DSI screen and translate touch input into gamepad state / HID reports.

The intended user may have no embedded development experience. Keep setup and usage reproducible from a fresh clone.

## Main Firmware Entry Point

Use this project as the main firmware:

```text
lvgl_demo_v9/
```

Important files:

```text
lvgl_demo_v9/main/main.c                  # ESP-IDF app_main, display start, UI create, HID start
lvgl_demo_v9/main/CMakeLists.txt          # Includes ../../src gamepad sources
src/xbox_touch_gamepad_layout_demo.c      # Main LVGL gamepad UI
src/gamepad_multitouch_input.c/.h         # GT911 multi-touch frame scanner / hit testing
src/gamepad_action_mapper.c/.h            # UI action -> input state mapping
src/gamepad_input_state.c/.h              # Gamepad state model
src/gamepad_output.c/.h                   # Output state polling
src/gamepad_hid_report.c/.h               # Gamepad state -> HID report
src/gamepad_hid_device.c/.h               # HID transport boundary task
src/gamepad_usb_descriptors.c/.h          # USB descriptor data
lvgl_demo_v9/components/espressif__esp_lvgl_port/src/lvgl9/esp_lvgl_port_touch.c
                                            # Local multi-touch hook into esp_lvgl_port
```

## Hardware Assumptions

- Board: WT9932P4-TINY / ESP32-P4
- Display: 7-inch 1024 x 600 MIPI-DSI LCD
- LCD driver: EK79007
- Touch controller: GT911
- ESP-IDF: 5.4.x
- LVGL: 9.2.x via ESP-IDF component manager

Hardware wiring is documented in `docs/HARDWARE.md`.

## Build Commands

Windows ESP-IDF CMD:

```bat
cd WT9932P4-TINY\lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
idf.py -p COM20 flash monitor
```

Portable helper script:

```bat
flash_com20.bat COM20
```

Replace `COM20` with the actual serial port.

## Validation Commands

Minimum validation before claiming success:

```bat
cd WT9932P4-TINY\lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

Expected result:

```text
Project build complete.
```

More validation steps are in `docs/VALIDATION.md`.

## Architectural Boundaries

Do not casually rewrite these boundaries:

1. `app_main()` starts BSP display first, then creates UI while LVGL is locked.
2. `lvgl_demo_v9/main/CMakeLists.txt` pulls reusable gamepad code from root `src/`.
3. GT911 multi-touch support is bridged from `esp_lvgl_port_touch.c` into `gamepad_multitouch_input_on_points()`.
4. When multi-touch mode is enabled, legacy single-pointer LVGL events must not fight the multi-touch scanner.
5. HID transport is isolated behind `gamepad_hid_device_start()` / `gamepad_hid_device_stop()`.
6. Local component overrides in `lvgl_demo_v9/components/` are intentional. Do not replace them with freshly downloaded components unless you re-apply local changes.

## Files Not To Commit

Do not commit generated build output:

```text
lvgl_demo_v9/build*/
**/managed_components/
*.log
```

The root `.gitignore` should already exclude these.

## Documentation Requirements

When changing setup, hardware, build, or flashing behavior, update:

```text
README.md
docs/SETUP_WINDOWS.md
docs/HARDWARE.md
docs/VALIDATION.md
```

When changing architecture or source layout, update:

```text
docs/PROJECT_MAP.md
AGENTS.md
CLAUDE.md
.github/copilot-instructions.md
```

## Common Pitfalls

- ESP32-P4 requires the RISC-V ESP toolchain that supports `xesploop` and `xespv`.
- Git Bash / MSYS may break ESP-IDF export scripts. For normal users, prefer Windows `ESP-IDF 5.4 CMD`.
- A lit blue/blank screen does not prove the UI ran. Check serial logs for `GamePad UI created`.
- Standard LVGL pointer input may only expose one point; this project has a separate multi-touch bridge for GT911.
- If touch init fails, display/UI should still be allowed to start where possible so display bring-up remains debuggable.
- Keep Chinese paths out of beginner instructions; suggest simple ASCII paths such as `D:\code\WT9932P4-TINY`.

## Response Style For AI Helpers

- Be concrete: cite file paths and commands.
- Do not invent board pins, components, or APIs. Inspect existing files first.
- Do not remove local BSP/LVGL component patches without checking their purpose.
- Prefer small, verifiable changes.
- Always run or request the relevant build before saying the firmware works.
