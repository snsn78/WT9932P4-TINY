#ifndef GAMEPAD_HID_DEVICE_H
#define GAMEPAD_HID_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USB HID transport boundary
 * ------------------------------------------------------------------
 * This module is the hardware-facing boundary behind the UI/action mapping
 * chain. It owns TinyUSB initialization and the polling/build/send loop:
 *
 *   gamepad_output_poll() -> gamepad_hid_report_build() -> TinyUSB
 *
 * It deliberately does not depend on GT911, LVGL widgets, BSP touch, or BLE.
 */

void gamepad_hid_device_start(void);
void gamepad_hid_device_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_HID_DEVICE_H */
