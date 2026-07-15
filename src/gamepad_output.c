#include "gamepad_output.h"

#include <stddef.h>

/* 输出层实现 (Step 12) */

void gamepad_output_build_from_state(const gamepad_input_state_t * state,
                                     gamepad_output_report_t * out)
{
    uint16_t buttons = 0;
    uint8_t dpad = 0;

    if(state == NULL || out == NULL) return;

    out->move_x = state->move_x;
    out->move_y = state->move_y;
    out->look_x = state->look_x;
    out->look_y = state->look_y;

    if(state->btn_a)      buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_A);
    if(state->btn_b)      buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_B);
    if(state->btn_x)      buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_X);
    if(state->btn_y)      buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_Y);
    if(state->btn_l1)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_L1);
    if(state->btn_l2)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_L2);
    if(state->btn_r1)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_R1);
    if(state->btn_r2)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_R2);
    if(state->btn_start)  buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_START);
    if(state->btn_select) buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_SELECT);
    if(state->btn_l3)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_L3);
    if(state->btn_r3)     buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_R3);
    if(state->btn_home)   buttons |= GAMEPAD_OUTPUT_BTN_BIT(GAMEPAD_BTN_HOME);

    if(state->dpad_up)    dpad |= GAMEPAD_OUTPUT_DPAD_UP;
    if(state->dpad_down)  dpad |= GAMEPAD_OUTPUT_DPAD_DOWN;
    if(state->dpad_left)  dpad |= GAMEPAD_OUTPUT_DPAD_LEFT;
    if(state->dpad_right) dpad |= GAMEPAD_OUTPUT_DPAD_RIGHT;

    out->buttons = buttons;
    out->dpad = dpad;
    out->seq = state->update_seq;
}

bool gamepad_output_poll(gamepad_output_report_t * out)
{
    const gamepad_input_state_t * state = gamepad_input_get_state();

    if(state == NULL || out == NULL) return false;

    gamepad_output_build_from_state(state, out);
    return true;
}
