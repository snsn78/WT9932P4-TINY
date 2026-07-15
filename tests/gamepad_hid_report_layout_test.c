#include <stddef.h>

#include "gamepad_hid_report.h"

_Static_assert(sizeof(gamepad_hid_report_t) == 7, "HID input report must be exactly 7 bytes");
_Static_assert(offsetof(gamepad_hid_report_t, left_x) == 0, "left_x offset mismatch");
_Static_assert(offsetof(gamepad_hid_report_t, left_y) == 1, "left_y offset mismatch");
_Static_assert(offsetof(gamepad_hid_report_t, right_x) == 2, "right_x offset mismatch");
_Static_assert(offsetof(gamepad_hid_report_t, right_y) == 3, "right_y offset mismatch");
_Static_assert(offsetof(gamepad_hid_report_t, buttons) == 4, "buttons offset mismatch");
_Static_assert(offsetof(gamepad_hid_report_t, hat) == 6, "hat offset mismatch");

int gamepad_hid_report_layout_test_translation_unit(void)
{
    return 0;
}
