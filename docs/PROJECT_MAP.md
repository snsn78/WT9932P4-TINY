# Project Map

This document maps the repository for humans and local AI coding assistants.

## Product Summary

The product is a touch-screen virtual gamepad firmware for WT9932P4-TINY / ESP32-P4.

It displays an LVGL gamepad UI on a 7-inch MIPI-DSI LCD and maps GT911 touch input into internal gamepad state and HID reports.

## Main Firmware Project

```text
lvgl_demo_v9/
```

Build from this directory with ESP-IDF 5.4.x and target `esp32p4`.

```bat
cd lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

## Top-Level Layout

```text
WT9932P4-TINY/
├── AGENTS.md                         # AI agent rules and project boundaries
├── CLAUDE.md                         # Claude-specific handoff instructions
├── .github/copilot-instructions.md   # GitHub Copilot repository instructions
├── README.md                         # Human-facing project overview
├── docs/
│   ├── HARDWARE.md                   # Hardware wiring and hardware troubleshooting
│   ├── PROJECT_MAP.md                # This file
│   ├── SETUP_WINDOWS.md              # Beginner Windows setup guide
│   └── VALIDATION.md                 # Build/flash/runtime acceptance checks
├── lvgl_demo_v9/                     # Main firmware project
├── src/                              # Reusable gamepad application sources
├── tests/                            # PC-side logic tests
├── common_components/                # Shared ESP-IDF components
├── blink/                            # Vendor LED example
├── lvgl_demo_v8/                     # Vendor LVGL v8 example
└── video_lcd_display/                # Vendor camera/display example
```

## Main Boot Flow

Entry point:

```text
lvgl_demo_v9/main/main.c
```

Current order:

1. Log `GamePad firmware booting`
2. Start BSP display with `bsp_display_start_with_config()`
3. Turn on backlight
4. Mount SPIFFS
5. Lock LVGL with `bsp_display_lock()`
6. Create the virtual gamepad UI with `xbox_touch_gamepad_layout_demo_create()`
7. Optionally run mapping self-test when `GAMEPAD_MAPPING_SELFTEST` is defined
8. Force initial refresh with `lv_refr_now(disp)`
9. Unlock LVGL
10. Start HID transport boundary with `gamepad_hid_device_start()`

## Source Modules

### UI Layer

```text
src/xbox_touch_gamepad_layout_demo.c
src/xbox_touch_gamepad_layout_demo.h
src/gamepad_module_builders.h
src/joystick_stick_demo.c
src/Dpad.c
src/shoulder_keys_demo.c
src/codex_gpt5_slider_2d_demo.c
```

Responsibilities:

- Build LVGL widgets for the gamepad UI
- Provide visual controls such as joystick, D-pad, shoulder buttons, sliders, and touch markers
- Bind UI controls to gamepad actions

### Layout Model

```text
src/gamepad_layout_model.c
src/gamepad_layout_model.h
```

Responsibilities:

- Define modules, positions, sizes, and action bindings
- Provide the runtime model used by UI builders and hit testing

### Input State

```text
src/gamepad_input_state.c
src/gamepad_input_state.h
```

Responsibilities:

- Store current button, D-pad, trigger, and analog state
- Provide mutation APIs used by action mapping and multi-touch logic

### Action Mapper

```text
src/gamepad_action_mapper.c
src/gamepad_action_mapper.h
```

Responsibilities:

- Convert UI actions into input-state changes
- Apply analog deadzone/sensitivity logic
- Avoid legacy single-pointer updates when the ESP multi-touch scanner is active

### Multi-Touch Bridge

```text
src/gamepad_multitouch_input.c
src/gamepad_multitouch_input.h
lvgl_demo_v9/components/espressif__esp_lvgl_port/src/lvgl9/esp_lvgl_port_touch.c
```

Responsibilities:

- Read all GT911 touch points from ESP-LVGL port
- Forward full touch frames into `gamepad_multitouch_input_on_points()`
- Hit-test multiple points against virtual controls
- Update gamepad input state so multiple controls can be pressed together
- Provide visual touch markers for debugging

Important: Standard LVGL pointer input usually exposes only one pointer. Do not remove this bridge unless replacing it with another true multi-touch path.

### Output / HID Mapping

```text
src/gamepad_output.c
src/gamepad_output.h
src/gamepad_hid_report.c
src/gamepad_hid_report.h
src/gamepad_hid_device.c
src/gamepad_hid_device.h
src/gamepad_usb_descriptors.c
src/gamepad_usb_descriptors.h
```

Responsibilities:

- Poll gamepad output state
- Convert state into HID report bytes
- Keep USB/HID transport isolated from UI code
- Define USB descriptor data for future/active HID transport work

### Self Tests

```text
src/gamepad_hid_selftest.c
src/gamepad_hid_selftest.h
src/gamepad_mapping_selftest.c
src/gamepad_mapping_selftest.h
tests/gamepad_hid_report_layout_test.c
tests/gamepad_multitouch_input_test.c
tests/gamepad_view_motion_test.c
```

Responsibilities:

- Validate HID report layout and mapping assumptions
- Validate multi-touch hit-test/input behavior on PC where possible
- Provide guardrails before flashing to hardware

## ESP-IDF Components

### Local Overrides

```text
lvgl_demo_v9/components/espressif__esp32_p4_function_ev_board/
lvgl_demo_v9/components/espressif__esp_lvgl_port/
```

These are intentionally committed. They preserve local BSP and LVGL-port changes needed by this project.

Do not delete them just because ESP-IDF can download managed components. If you replace them, you must re-apply and re-verify the local changes.

### Managed Components

```text
lvgl_demo_v9/managed_components/
```

This directory is generated by ESP-IDF Component Manager and should not be committed.

## Documentation Map

- `README.md`: public project overview and quick start
- `docs/SETUP_WINDOWS.md`: beginner setup, downloads, build, flash
- `docs/HARDWARE.md`: wiring and hardware troubleshooting
- `docs/VALIDATION.md`: build, flash, runtime acceptance checks
- `AGENTS.md`: local AI/model instructions
- `CLAUDE.md`: Claude Code specific instructions
- `.github/copilot-instructions.md`: GitHub Copilot context

## Safe Extension Points

Prefer adding new features in these areas:

- New UI controls: `src/xbox_touch_gamepad_layout_demo.c` or separate `src/gamepad_*` modules
- New action types: `src/gamepad_layout_model.h`, `src/gamepad_action_mapper.c`, `src/gamepad_input_state.*`
- New HID outputs: `src/gamepad_hid_report.*`, `src/gamepad_usb_descriptors.*`
- New validation: `tests/` and self-test modules under `src/`

## Risky Areas

Be careful when modifying:

- `lvgl_demo_v9/components/espressif__esp_lvgl_port/src/lvgl9/esp_lvgl_port_touch.c`
- `lvgl_demo_v9/components/espressif__esp32_p4_function_ev_board/`
- `lvgl_demo_v9/sdkconfig`
- `lvgl_demo_v9/dependencies.lock`
- `lvgl_demo_v9/main/CMakeLists.txt`

Changes there can break display startup, touch registration, dependency resolution, or ESP-IDF builds.
