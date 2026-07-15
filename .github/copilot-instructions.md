# GitHub Copilot Instructions

This repository contains an ESP32-P4 / LVGL 9 touch-screen virtual gamepad project.

Read these files for context before making suggestions:

- `AGENTS.md`
- `docs/PROJECT_MAP.md`
- `docs/VALIDATION.md`
- `docs/SETUP_WINDOWS.md`
- `docs/HARDWARE.md`

## Main Firmware

Use `lvgl_demo_v9/` as the main firmware project.

Important source areas:

- `lvgl_demo_v9/main/main.c` — app entry point and boot sequence
- `lvgl_demo_v9/main/CMakeLists.txt` — source list for root `src/`
- `src/` — virtual gamepad UI, input state, multi-touch, HID mapping
- `lvgl_demo_v9/components/` — local component overrides, intentionally committed

## Build

```bat
cd lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

## Do Not

- Do not remove local patches in `lvgl_demo_v9/components/`.
- Do not commit generated `build*` directories or `managed_components`.
- Do not assume LVGL single-pointer input is enough for the gamepad; this project needs GT911 multi-touch bridging.
- Do not move `lvgl_demo_v9` away from the repository root because its CMake file references `../../src`.
