# WT9932P4-TINY LVGL v9 Local AI Run Guide

This project is prepared so another developer or local AI agent can clone the
repository and build the LVGL gamepad demo without depending on files outside
the repository.

## Hardware

- Board: WT9932P4-TINY / ESP32-P4
- Display: MIPI-DSI EK79007, 1024 x 600
- Touch: GT911
- ESP-IDF: v5.4.x
- LVGL: v9.2.x

## Repository Layout

Keep this layout intact:

```text
WT9932P4-TINY/
+-- common_components/
+-- src/                         # Gamepad UI/application sources
+-- lvgl_demo_v9/
    +-- components/              # Local component overrides
    +-- main/
    +-- CMakeLists.txt
    +-- sdkconfig.defaults
    +-- partitions.csv
```

`lvgl_demo_v9/main/CMakeLists.txt` references `../../src`, so `lvgl_demo_v9`
must remain inside the repository root.

The two components under `lvgl_demo_v9/components/` are intentionally committed.
They preserve local changes that would otherwise be lost if ESP-IDF downloaded
fresh managed components.

## Build

Use a normal CMD terminal:

```bat
cd /d <ESP-IDF>
install.bat esp32p4
export.bat
cd /d <repo>\lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

Flash example:

```bat
idf.py -p COM20 flash monitor
```

Exit monitor with `Ctrl + ]`.

## AI Constraints

When asking a local AI to continue this project:

- Preserve the BSP display startup sequence in `main/main.c`.
- Do not reimplement MIPI-DSI, EK79007, GT911, LVGL tick, or LVGL handler tasks.
- Keep initial LVGL UI creation inside `bsp_display_lock()` and
  `bsp_display_unlock()`.
- Add new gamepad UI code under repository `src/` or a project component.
- Do not depend on source files outside this repository.
