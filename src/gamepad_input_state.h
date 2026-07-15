#ifndef GAMEPAD_INPUT_STATE_H
#define GAMEPAD_INPUT_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 统一输入状态层 (Step 4)
 * ------------------------------------------------------------------
 * 所有虚拟手柄控件(左摇杆/视角滑块/DPad/肩键/按钮)都只通过本层写入状态，
 * 后续的动作映射层、编辑器、输出层都从本层读取，形成唯一数据收口。
 *
 * 全局符号约定 (Step 3 冻结，务必所有模块统一)：
 *   - X 轴：右为 +，左为 -
 *   - Y 轴：上为 +，下为 -   (数学/视角习惯，与摇杆 display_y、滑块 y_percent 一致)
 *   - 模拟量范围：[-1.0f, 1.0f]
 *   - 离散量：按下 = true，抬起 = false
 *
 * 线程模型：LVGL 单线程(在 bsp_display_lock 保护下运行)。本层不加锁，
 * 输出层若从其它任务读取，应在读取侧自行做一致性快照。
 */

/* 按键 ID：与 Xbox 语义一致的离散按钮集合 */
typedef enum {
    GAMEPAD_BTN_A = 0,
    GAMEPAD_BTN_B,
    GAMEPAD_BTN_X,
    GAMEPAD_BTN_Y,
    GAMEPAD_BTN_L1,
    GAMEPAD_BTN_L2,
    GAMEPAD_BTN_R1,
    GAMEPAD_BTN_R2,
    GAMEPAD_BTN_START,
    GAMEPAD_BTN_SELECT,
    GAMEPAD_BTN_L3,
    GAMEPAD_BTN_R3,
    GAMEPAD_BTN_HOME,
    GAMEPAD_BTN_COUNT
} gamepad_button_id_t;

/* DPad 四方向 */
typedef enum {
    GAMEPAD_DPAD_UP = 0,
    GAMEPAD_DPAD_DOWN,
    GAMEPAD_DPAD_LEFT,
    GAMEPAD_DPAD_RIGHT,
    GAMEPAD_DPAD_COUNT
} gamepad_dpad_dir_t;

/* 统一输入状态快照 */
typedef struct {
    float move_x;   /* 左摇杆 X，[-1,1]，右为 + */
    float move_y;   /* 左摇杆 Y，[-1,1]，上为 + */
    float look_x;   /* 视角 X，[-1,1]，右为 + */
    float look_y;   /* 视角 Y，[-1,1]，上为 + */

    bool btn_a;
    bool btn_b;
    bool btn_x;
    bool btn_y;
    bool btn_l1;
    bool btn_l2;
    bool btn_r1;
    bool btn_r2;
    bool btn_start;
    bool btn_select;
    bool btn_l3;
    bool btn_r3;
    bool btn_home;

    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;

    /* 每次状态发生实际变化时递增，供观察层/输出层做脏检测 */
    uint32_t update_seq;
} gamepad_input_state_t;

/* 复位为全零(松手态)。update_seq 会自增一次。 */
void gamepad_input_state_reset(void);

/* 只读获取当前状态快照指针(常驻，勿修改)。 */
const gamepad_input_state_t * gamepad_input_get_state(void);

/* 写入左摇杆(移动)模拟量。入参会被 clamp 到 [-1,1]。 */
void gamepad_input_set_move(float x, float y);

/* 写入视角模拟量。入参会被 clamp 到 [-1,1]。 */
void gamepad_input_set_look(float x, float y);

/* 写入某个离散按钮的按下态。 */
void gamepad_input_set_button(gamepad_button_id_t button, bool pressed);

/* 写入某个 DPad 方向的按下态。 */
void gamepad_input_set_dpad(gamepad_dpad_dir_t dir, bool pressed);

/* 调试辅助：按钮 ID 转字符串。 */
const char * gamepad_button_id_to_string(gamepad_button_id_t button);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_INPUT_STATE_H */
