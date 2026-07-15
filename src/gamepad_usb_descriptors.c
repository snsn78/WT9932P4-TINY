#include "gamepad_usb_descriptors.h"

#include <stddef.h>

#include "class/hid/hid_device.h"
#include "tusb.h"

#define GAMEPAD_USB_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define GAMEPAD_USB_EP_IN            0x81
#define GAMEPAD_USB_EP_SIZE          16

const uint8_t gamepad_hid_report_descriptor[] = {
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x05,       /* Usage (Game Pad) */
    0xA1, 0x01,       /* Collection (Application) */

    0x09, 0x30,       /* Usage (X) */
    0x09, 0x31,       /* Usage (Y) */
    0x09, 0x33,       /* Usage (Rx) */
    0x09, 0x34,       /* Usage (Ry) */
    0x15, 0x81,       /* Logical Minimum (-127) */
    0x25, 0x7F,       /* Logical Maximum (127) */
    0x75, 0x08,       /* Report Size (8) */
    0x95, 0x04,       /* Report Count (4) */
    0x81, 0x02,       /* Input (Data, Variable, Absolute) */

    0x05, 0x09,       /* Usage Page (Button) */
    0x19, 0x01,       /* Usage Minimum (Button 1) */
    0x29, 0x0D,       /* Usage Maximum (Button 13) */
    0x15, 0x00,       /* Logical Minimum (0) */
    0x25, 0x01,       /* Logical Maximum (1) */
    0x75, 0x01,       /* Report Size (1) */
    0x95, 0x0D,       /* Report Count (13) */
    0x81, 0x02,       /* Input (Data, Variable, Absolute) */
    0x75, 0x03,       /* Report Size (3) */
    0x95, 0x01,       /* Report Count (1) */
    0x81, 0x03,       /* Input (Constant, Variable, Absolute) */

    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x39,       /* Usage (Hat Switch) */
    0x15, 0x00,       /* Logical Minimum (0) */
    0x25, 0x07,       /* Logical Maximum (7) */
    0x35, 0x00,       /* Physical Minimum (0) */
    0x46, 0x3B, 0x01, /* Physical Maximum (315) */
    0x65, 0x14,       /* Unit (Degrees) */
    0x75, 0x04,       /* Report Size (4) */
    0x95, 0x01,       /* Report Count (1) */
    0x81, 0x42,       /* Input (Data, Variable, Absolute, Null State) */
    0x65, 0x00,       /* Unit (None) */
    0x75, 0x04,       /* Report Size (4) */
    0x95, 0x01,       /* Report Count (1) */
    0x81, 0x03,       /* Input (Constant, Variable, Absolute) */

    0xC0              /* End Collection */
};

const char *gamepad_usb_string_descriptor[] = {
    (const char[]){0x09, 0x04},
    "ESPRESSIF",
    "ESP32-P4 USB Gamepad",
    "ESP32P4-GAMEPAD",
    "Gamepad HID Interface",
};

const uint8_t gamepad_usb_string_descriptor_count =
    sizeof(gamepad_usb_string_descriptor) / sizeof(gamepad_usb_string_descriptor[0]);

const uint8_t gamepad_usb_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, GAMEPAD_USB_CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(gamepad_hid_report_descriptor),
                       GAMEPAD_USB_EP_IN, GAMEPAD_USB_EP_SIZE, 5),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return gamepad_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t requested_length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)requested_length;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t buffer_size)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)buffer_size;
}
