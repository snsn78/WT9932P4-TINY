#include "gamepad_hid_report.h"

#include <stddef.h>
#include <stdbool.h>

/* HID report 适配层实现 (Step 12) */

static int8_t axis_to_int8(float v)
{
    /* 状态层为 [-1,1]，映射到 [-127,127] */
    int32_t scaled;

    if(v > 1.0f) v = 1.0f;
    if(v < -1.0f) v = -1.0f;
    scaled = (int32_t)(v * 127.0f + (v >= 0.0f ? 0.5f : -0.5f));
    if(scaled > 127) scaled = 127;
    if(scaled < -127) scaled = -127;
    return (int8_t)scaled;
}

/* 由 4 位方向掩码求 8 方向 hat；居中返回 0x0F。 */
static uint8_t dpad_to_hat(uint8_t dpad)
{
    const bool up = (dpad & GAMEPAD_OUTPUT_DPAD_UP) != 0;
    const bool down = (dpad & GAMEPAD_OUTPUT_DPAD_DOWN) != 0;
    const bool left = (dpad & GAMEPAD_OUTPUT_DPAD_LEFT) != 0;
    const bool right = (dpad & GAMEPAD_OUTPUT_DPAD_RIGHT) != 0;

    if(up && right)   return 1;
    if(down && right) return 3;
    if(down && left)  return 5;
    if(up && left)    return 7;
    if(up)            return 0;
    if(right)         return 2;
    if(down)          return 4;
    if(left)          return 6;
    return GAMEPAD_HID_HAT_CENTER;
}

void gamepad_hid_report_build(const gamepad_output_report_t * out, gamepad_hid_report_t * hid)
{
    if(out == NULL || hid == NULL) return;

    hid->left_x = axis_to_int8(out->move_x);
    hid->left_y = axis_to_int8(-out->move_y);   /* 上为+ -> HID 下为+，翻转 */
    hid->right_x = axis_to_int8(out->look_x);
    hid->right_y = axis_to_int8(-out->look_y);
    hid->buttons = out->buttons;
    hid->hat = dpad_to_hat(out->dpad);
}
