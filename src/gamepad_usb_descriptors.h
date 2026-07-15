#ifndef GAMEPAD_USB_DESCRIPTORS_H
#define GAMEPAD_USB_DESCRIPTORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t gamepad_hid_report_descriptor[];
extern const uint8_t gamepad_usb_configuration_descriptor[];
extern const char *gamepad_usb_string_descriptor[];
extern const uint8_t gamepad_usb_string_descriptor_count;

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_USB_DESCRIPTORS_H */
