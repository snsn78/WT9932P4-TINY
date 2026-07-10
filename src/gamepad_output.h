#ifndef GAMEPAD_OUTPUT_H
#define GAMEPAD_OUTPUT_H

#include <stdint.h>

#include "gamepad_input_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 输出层 (Step 12)
 * ------------------------------------------------------------------
 * 把统一输入状态层(gamepad_input_state)标准化为一个与 UI 完全解耦的输出报文，
 * 供业务逻辑 / 虚拟手柄协议 / HID 适配层消费。本层不反向依赖任何 LVGL/UI 控件。
 *
 * 位布局：buttons 中每个按钮占一位，位号 = gamepad_button_id_t 的枚举值。
 *         dpad 用 4 位方向掩码。
 */

#define GAMEPAD_OUTPUT_BTN_BIT(id)   ((uint16_t)(1u << (id)))

#define GAMEPAD_OUTPUT_DPAD_UP       (1u << 0)
#define GAMEPAD_OUTPUT_DPAD_DOWN     (1u << 1)
#define GAMEPAD_OUTPUT_DPAD_LEFT     (1u << 2)
#define GAMEPAD_OUTPUT_DPAD_RIGHT    (1u << 3)

typedef struct {
    float move_x;      /* [-1,1] 右为 + */
    float move_y;      /* [-1,1] 上为 + */
    float look_x;      /* [-1,1] 右为 + */
    float look_y;      /* [-1,1] 上为 + */
    uint16_t buttons;  /* 按位：见 GAMEPAD_OUTPUT_BTN_BIT(gamepad_button_id_t) */
    uint8_t dpad;      /* 按位：见 GAMEPAD_OUTPUT_DPAD_* */
    uint32_t seq;      /* 透传自状态层的 update_seq，供消费侧做变化检测 */
} gamepad_output_report_t;

/* 由给定状态快照构建输出报文(纯转换，无副作用)。 */
void gamepad_output_build_from_state(const gamepad_input_state_t * state,
                                     gamepad_output_report_t * out);

/* 便捷：读取当前全局状态并构建报文。返回是否成功。 */
bool gamepad_output_poll(gamepad_output_report_t * out);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_OUTPUT_H */
