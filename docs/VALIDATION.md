# Validation Guide

Use this guide to verify the project after cloning, editing, or preparing a release.

## 1. Fresh Clone Validation

From a clean machine or clean folder:

```bat
git clone https://github.com/snsn78/WT9932P4-TINY.git
cd WT9932P4-TINY\lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
```

Expected final output:

```text
Project build complete.
```

Expected generated files:

```text
lvgl_demo_v9\build\lvgl_demo_v9.bin
lvgl_demo_v9\build\lvgl_demo_v9.elf
lvgl_demo_v9\build\bootloader\bootloader.bin
lvgl_demo_v9\build\partition_table\partition-table.bin
```

If using the helper script:

```bat
flash_com20.bat COM20
```

Replace `COM20` with the real serial port.

## 2. Build Environment Requirements

Known-good baseline:

- OS: Windows 10 / Windows 11
- ESP-IDF: 5.4.x
- Target: `esp32p4`
- Python: installed by ESP-IDF installer
- Toolchain: RISC-V ESP toolchain with ESP32-P4 extension support

Important: if the compiler fails with errors about unsupported `xesploop` or `xespv`, the RISC-V toolchain is too old. Re-run ESP-IDF installation for `esp32p4` or use ESP-IDF 5.4.x installer.

## 3. Flash Validation

With the board connected:

```bat
idf.py -p COM20 flash
```

Expected:

- Flash completes without connection errors
- Board resets after flashing
- No old firmware is accidentally flashed after a failed build

If flashing fails:

1. Close all serial monitors
2. Check the COM port in Device Manager
3. Try another USB-C data cable
4. Hold `BOOT`, tap `RESET`, release `RESET`, release `BOOT`, then flash again

## 4. Serial Monitor Validation

Open monitor:

```bat
idf.py -p COM20 monitor
```

Exit monitor:

```text
Ctrl + ]
```

Expected log markers:

```text
GamePad firmware booting
Starting BSP display
Display started
Backlight on
Mounting SPIFFS
Creating GamePad UI
GamePad UI created
Initial LVGL refresh requested
LVGL unlocked
HID transport boundary started
```

Touch-related logs may include GT911 status and multi-touch frame count.

## 5. Screen Validation

After flashing:

- LCD backlight turns on
- Virtual gamepad UI appears
- UI is not just a blank blue/black/white screen
- Touch markers or controls respond when touching the screen
- Multiple touch points can activate multiple controls where supported

## 6. Touch Validation

Touch validation requires working GT911 hardware.

Check:

- A single tap updates the corresponding virtual control
- Two fingers can press two independent controls without one cancelling the other
- D-pad and buttons can be held at the same time where layout permits
- Analog/slider controls move smoothly

If UI displays but touch does not work:

1. Check GT911 touch cable orientation
2. Check screen/touch board connection
3. Confirm the panel uses GT911
4. Read monitor logs for GT911 initialization errors
5. Check `esp_lvgl_port_touch.c` multi-touch hook remains present

## 7. HID / Gamepad State Validation

Current firmware includes the HID report and transport boundary modules.

Code-level checks:

- `gamepad_hid_device_start()` is called after LVGL unlock in `main.c`
- `gamepad_output_poll()` can produce updated output states
- `gamepad_hid_report_build()` converts output state into HID report data
- USB descriptor data remains isolated in `gamepad_usb_descriptors.*`

Runtime HID behavior depends on the current USB descriptor/device implementation and the board's USB connection mode. If extending HID transport, keep UI and touch logic separate from transport code.

## 8. Documentation Validation

Before publishing or handing off to another AI/user, confirm these files exist and are current:

```text
README.md
AGENTS.md
CLAUDE.md
.github/copilot-instructions.md
docs/SETUP_WINDOWS.md
docs/HARDWARE.md
docs/PROJECT_MAP.md
docs/VALIDATION.md
```

The README should point beginners to `docs/SETUP_WINDOWS.md`.

## 9. Git Hygiene Validation

Before committing:

```bat
git status --short
git diff --stat
```

Do not commit:

```text
lvgl_demo_v9\build\
lvgl_demo_v9\build_*\
lvgl_demo_v9\managed_components\
*.log
```

The root `.gitignore` should exclude these.

Optional sensitive text scan:

```bat
git diff --cached | findstr /i "token password secret api_key github_pat gho_"
```

Expected: no real credentials.

## 10. Known Non-Fatal Warnings

These may appear and are not necessarily fatal:

- ESP-IDF dependency notice that newer component versions are available
- Kconfig warning that `BSP_I2S_NUM` is defined in multiple locations
- Git executable not found in some automated build wrappers

Do not ignore actual compile errors or link errors.

## 11. Release Checklist

- [ ] Fresh build passes with `idf.py build`
- [ ] Build artifacts are ignored by Git
- [ ] README quick start is accurate
- [ ] Windows beginner setup doc is accurate
- [ ] Hardware wiring doc matches the target board and screen
- [ ] AI instruction files exist for local models
- [ ] No secrets are staged
- [ ] Changes are pushed to GitHub
