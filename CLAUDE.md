# Claude Code Instructions

This repository is an ESP32-P4 / LVGL 9 touch-screen virtual gamepad firmware.

Before editing, read:

1. `AGENTS.md` — project-wide AI instructions and boundaries
2. `docs/PROJECT_MAP.md` — source layout and module responsibilities
3. `docs/VALIDATION.md` — build, flash, and hardware acceptance checks
4. `docs/SETUP_WINDOWS.md` — beginner setup path
5. `docs/HARDWARE.md` — board and screen wiring

## Main Task Context

The main firmware project is:

```text
lvgl_demo_v9/
```

Build with ESP-IDF 5.4.x for target `esp32p4`:

```bat
cd lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

Do not treat `blink`, `lvgl_demo_v8`, or `video_lcd_display` as the main product. They are retained examples.

## Critical Rules

- Keep the repository cloneable and buildable from a fresh machine.
- Do not commit ESP-IDF build directories or downloaded `managed_components`.
- Preserve local component overrides under `lvgl_demo_v9/components/`.
- Do not bypass the multi-touch bridge by relying only on LVGL single-pointer events.
- If you change setup/build/hardware behavior, update the docs in the same change.
- Verify with `idf.py build` before claiming firmware success.
