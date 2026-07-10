# LVGL v9 Display-Only Test (No GT911)

This folder is a small ESP-IDF test project for bringing up the WT9932P4-TINY LCD first.

It intentionally does not initialize the GT911 touch controller, does not mount SPIFFS, and does not build the gamepad UI. The expected result is a 1024 x 600 LVGL color-bar screen with the backlight enabled.

## Build

```bat
cd /d E:\esp\esp-idf
export.bat
cd /d E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9_no_gt911
idf.py build
```

## Flash

Replace `COMx` with your ESP32-P4 serial port.

```bat
idf.py -p COMx flash monitor
```

If the screen lights and shows the test pattern, the LCD/MIPI/backlight path is alive. Touch can be handled later in the main `lvgl_demo_v9` project.
