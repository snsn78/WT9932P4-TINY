#include "gamepad_mapping_selftest.h"

#include "gamepad_input_state.h"
#include "gamepad_output.h"
#include "gamepad_hid_report.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#if defined(ESP_PLATFORM)
#include "esp_log.h"
static const char * SELFTEST_TAG = "gamepad_selftest";
#define SELFTEST_LOG(fmt, ...) ESP_LOGI(SELFTEST_TAG, fmt, ##__VA_ARGS__)
#else
#define SELFTEST_LOG(fmt, ...) printf("[gamepad_selftest] " fmt "\n", ##__VA_ARGS__)
#endif

typedef struct {
    gamepad_button_id_t button;
    const char * name;
} selftest_button_case_t;

typedef struct {
    const char * name;
    bool up;
    bool down;
    bool left;
    bool right;
    uint8_t expected_hat;
} selftest_dpad_case_t;

static bool build_reports(gamepad_output_report_t * out, gamepad_hid_report_t * hid)
{
    if(!gamepad_output_poll(out)) {
        SELFTEST_LOG("ERROR: gamepad_output_poll failed");
        return false;
    }
    gamepad_hid_report_build(out, hid);
    return true;
}

static void log_button_case(gamepad_button_id_t button, const char * name)
{
    gamepad_output_report_t out;
    gamepad_hid_report_t hid;
    const uint16_t expected = GAMEPAD_OUTPUT_BTN_BIT(button);

    gamepad_input_state_reset();
    gamepad_input_set_button(button, true);

    if(!build_reports(&out, &hid)) return;

    SELFTEST_LOG("BUTTON %-5s out.buttons=0x%04X hid.buttons=0x%04X expected_bit=0x%04X result=%s",
                 name,
                 (unsigned)out.buttons,
                 (unsigned)hid.buttons,
                 (unsigned)expected,
                 ((out.buttons & expected) && (hid.buttons & expected)) ? "PASS" : "FAIL");
}

static void set_dpad_case(const selftest_dpad_case_t * c)
{
    gamepad_input_set_dpad(GAMEPAD_DPAD_UP, c->up);
    gamepad_input_set_dpad(GAMEPAD_DPAD_DOWN, c->down);
    gamepad_input_set_dpad(GAMEPAD_DPAD_LEFT, c->left);
    gamepad_input_set_dpad(GAMEPAD_DPAD_RIGHT, c->right);
}

static void log_dpad_case(const selftest_dpad_case_t * c)
{
    gamepad_output_report_t out;
    gamepad_hid_report_t hid;

    gamepad_input_state_reset();
    set_dpad_case(c);

    if(!build_reports(&out, &hid)) return;

    SELFTEST_LOG("DPAD   %-10s out.dpad=0x%02X hid.hat=0x%02X expected_hat=0x%02X result=%s",
                 c->name,
                 (unsigned)out.dpad,
                 (unsigned)hid.hat,
                 (unsigned)c->expected_hat,
                 (hid.hat == c->expected_hat) ? "PASS" : "FAIL");
}

static void log_axis_case(void)
{
    gamepad_output_report_t out;
    gamepad_hid_report_t hid;

    gamepad_input_state_reset();
    gamepad_input_set_move(0.50f, 1.00f);
    gamepad_input_set_look(-1.00f, -0.50f);

    if(!build_reports(&out, &hid)) return;

    SELFTEST_LOG("AXIS   MOVE out=(%.2f, %.2f) hid.left=(%d, %d) expected_y_flip=-127 result=%s",
                 (double)out.move_x,
                 (double)out.move_y,
                 (int)hid.left_x,
                 (int)hid.left_y,
                 (hid.left_x == 64 && hid.left_y == -127) ? "PASS" : "CHECK");
    SELFTEST_LOG("AXIS   LOOK out=(%.2f, %.2f) hid.right=(%d, %d) expected_y_flip=64 result=%s",
                 (double)out.look_x,
                 (double)out.look_y,
                 (int)hid.right_x,
                 (int)hid.right_y,
                 (hid.right_x == -127 && hid.right_y == 64) ? "PASS" : "CHECK");
}

void gamepad_mapping_selftest_run_once(void)
{
    static bool ran_once = false;

    static const selftest_button_case_t button_cases[] = {
        { GAMEPAD_BTN_A, "A" },
        { GAMEPAD_BTN_B, "B" },
        { GAMEPAD_BTN_X, "X" },
        { GAMEPAD_BTN_Y, "Y" },
        { GAMEPAD_BTN_L1, "L1" },
        { GAMEPAD_BTN_L2, "L2" },
        { GAMEPAD_BTN_R1, "R1" },
        { GAMEPAD_BTN_R2, "R2" },
        { GAMEPAD_BTN_L3, "L3" },
        { GAMEPAD_BTN_R3, "R3" },
        { GAMEPAD_BTN_HOME, "HOME" },
    };
    static const selftest_dpad_case_t dpad_cases[] = {
        { "CENTER", false, false, false, false, GAMEPAD_HID_HAT_CENTER },
        { "UP", true, false, false, false, 0 },
        { "UP_RIGHT", true, false, false, true, 1 },
        { "RIGHT", false, false, false, true, 2 },
        { "DOWN_RIGHT", false, true, false, true, 3 },
        { "DOWN", false, true, false, false, 4 },
        { "DOWN_LEFT", false, true, true, false, 5 },
        { "LEFT", false, false, true, false, 6 },
        { "UP_LEFT", true, false, true, false, 7 },
    };

    size_t i;

    if(ran_once) {
        return;
    }
    ran_once = true;

    SELFTEST_LOG("START GT911-independent mapping selftest");

    gamepad_input_state_reset();

    for(i = 0U; i < sizeof(button_cases) / sizeof(button_cases[0]); ++i) {
        log_button_case(button_cases[i].button, button_cases[i].name);
    }

    for(i = 0U; i < sizeof(dpad_cases) / sizeof(dpad_cases[0]); ++i) {
        log_dpad_case(&dpad_cases[i]);
    }

    log_axis_case();

    gamepad_input_state_reset();
    SELFTEST_LOG("END state reset to released/centered");
}
