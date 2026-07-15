#ifndef GAMEPAD_MAPPING_SELFTEST_H
#define GAMEPAD_MAPPING_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Optional mapping-chain selftest for the GT911-disabled phase.
 *
 * This entry injects state directly through gamepad_input_state setters, then
 * builds gamepad_output_report_t and gamepad_hid_report_t snapshots. It does
 * not depend on GT911, LVGL touch indev, BSP touch code, or real USB/BLE HID
 * sending. Call it only from code guarded by GAMEPAD_MAPPING_SELFTEST.
 */
void gamepad_mapping_selftest_run_once(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_MAPPING_SELFTEST_H */
