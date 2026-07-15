#ifndef GAMEPAD_HID_SELFTEST_H
#define GAMEPAD_HID_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Define GAMEPAD_HID_SELFTEST at compile time to start this synthetic input loop. */
void gamepad_hid_selftest_start(void);
void gamepad_hid_selftest_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_HID_SELFTEST_H */
