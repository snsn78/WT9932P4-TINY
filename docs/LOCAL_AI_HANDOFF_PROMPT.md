# Local AI Handoff Prompt

Copy the prompt below into a local LLM / coding assistant after opening this repository.

```text
You are helping with an ESP32-P4 / LVGL 9 touch-screen virtual gamepad firmware project.

First read these files:

1. AGENTS.md
2. docs/PROJECT_MAP.md
3. docs/VALIDATION.md
4. docs/SETUP_WINDOWS.md
5. docs/HARDWARE.md
6. README.md

Project goal:
- Build firmware for WT9932P4-TINY / ESP32-P4.
- Display a virtual gamepad UI on a 7-inch 1024x600 MIPI-DSI EK79007 screen.
- Use GT911 touch input, including multi-touch, to update gamepad state.
- Keep HID report / USB transport logic isolated from UI code.

Main firmware directory:
- lvgl_demo_v9/

Build commands:
cd lvgl_demo_v9
idf.py set-target esp32p4
idf.py build

Important constraints:
- Do not remove lvgl_demo_v9/components local overrides.
- Do not rely only on LVGL single-pointer events for multi-touch controls.
- Do not commit build directories, managed_components, logs, or local IDE files.
- Keep lvgl_demo_v9 at this relative location because it includes ../../src.
- If changing setup/build/hardware behavior, update README.md and docs/*.md.
- Verify with idf.py build before saying the firmware works.

When answering or editing, cite exact file paths and commands.
```

## Recommended First Question

After giving the prompt above, ask the local AI:

```text
Please inspect the repository using AGENTS.md and docs/PROJECT_MAP.md, then summarize the build entry point, key modules, and validation steps. Do not edit files yet.
```

This forces the model to load context before making changes.
