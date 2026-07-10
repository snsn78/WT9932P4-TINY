#include "gamepad_input_state.h"

#include <stddef.h>   /* NULL */

/* 统一输入状态层实现 (Step 4) */

static gamepad_input_state_t g_state;

static float clampf(float v, float lo, float hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static void bump_seq(void)
{
    g_state.update_seq++;
}

void gamepad_input_state_reset(void)
{
    /* 逐字段清零，保留自增语义 */
    g_state.move_x = 0.0f;
    g_state.move_y = 0.0f;
    g_state.look_x = 0.0f;
    g_state.look_y = 0.0f;

    g_state.btn_a = false;
    g_state.btn_b = false;
    g_state.btn_x = false;
    g_state.btn_y = false;
    g_state.btn_l1 = false;
    g_state.btn_l2 = false;
    g_state.btn_r1 = false;
    g_state.btn_r2 = false;
    g_state.btn_start = false;
    g_state.btn_select = false;

    g_state.dpad_up = false;
    g_state.dpad_down = false;
    g_state.dpad_left = false;
    g_state.dpad_right = false;

    bump_seq();
}

const gamepad_input_state_t * gamepad_input_get_state(void)
{
    return &g_state;
}

void gamepad_input_set_move(float x, float y)
{
    const float nx = clampf(x, -1.0f, 1.0f);
    const float ny = clampf(y, -1.0f, 1.0f);

    if(nx == g_state.move_x && ny == g_state.move_y) {
        return;
    }
    g_state.move_x = nx;
    g_state.move_y = ny;
    bump_seq();
}

void gamepad_input_set_look(float x, float y)
{
    const float nx = clampf(x, -1.0f, 1.0f);
    const float ny = clampf(y, -1.0f, 1.0f);

    if(nx == g_state.look_x && ny == g_state.look_y) {
        return;
    }
    g_state.look_x = nx;
    g_state.look_y = ny;
    bump_seq();
}

void gamepad_input_set_button(gamepad_button_id_t button, bool pressed)
{
    bool * slot = NULL;

    switch(button) {
        case GAMEPAD_BTN_A:      slot = &g_state.btn_a; break;
        case GAMEPAD_BTN_B:      slot = &g_state.btn_b; break;
        case GAMEPAD_BTN_X:      slot = &g_state.btn_x; break;
        case GAMEPAD_BTN_Y:      slot = &g_state.btn_y; break;
        case GAMEPAD_BTN_L1:     slot = &g_state.btn_l1; break;
        case GAMEPAD_BTN_L2:     slot = &g_state.btn_l2; break;
        case GAMEPAD_BTN_R1:     slot = &g_state.btn_r1; break;
        case GAMEPAD_BTN_R2:     slot = &g_state.btn_r2; break;
        case GAMEPAD_BTN_START:  slot = &g_state.btn_start; break;
        case GAMEPAD_BTN_SELECT: slot = &g_state.btn_select; break;
        default: return;
    }

    if(*slot == pressed) {
        return;
    }
    *slot = pressed;
    bump_seq();
}

void gamepad_input_set_dpad(gamepad_dpad_dir_t dir, bool pressed)
{
    bool * slot = NULL;

    switch(dir) {
        case GAMEPAD_DPAD_UP:    slot = &g_state.dpad_up; break;
        case GAMEPAD_DPAD_DOWN:  slot = &g_state.dpad_down; break;
        case GAMEPAD_DPAD_LEFT:  slot = &g_state.dpad_left; break;
        case GAMEPAD_DPAD_RIGHT: slot = &g_state.dpad_right; break;
        default: return;
    }

    if(*slot == pressed) {
        return;
    }
    *slot = pressed;
    bump_seq();
}

const char * gamepad_button_id_to_string(gamepad_button_id_t button)
{
    switch(button) {
        case GAMEPAD_BTN_A:      return "A";
        case GAMEPAD_BTN_B:      return "B";
        case GAMEPAD_BTN_X:      return "X";
        case GAMEPAD_BTN_Y:      return "Y";
        case GAMEPAD_BTN_L1:     return "L1";
        case GAMEPAD_BTN_L2:     return "L2";
        case GAMEPAD_BTN_R1:     return "R1";
        case GAMEPAD_BTN_R2:     return "R2";
        case GAMEPAD_BTN_START:  return "START";
        case GAMEPAD_BTN_SELECT: return "SELECT";
        default:                 return "?";
    }
}
