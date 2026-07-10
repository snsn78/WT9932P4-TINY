#ifndef GAMEPAD_ACTION_MAPPER_H
#define GAMEPAD_ACTION_MAPPER_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "gamepad_input_state.h"
#include "gamepad_layout_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 运行时动作映射层 (Step 10)
 * ------------------------------------------------------------------
 * 目的：把“控件”与“它控制的动作”解耦。控件本身不再写死自己是 A 还是移动，
 * 而是把归一化后的输入(模拟向量 / 离散按下)连同自身对象树上报给 mapper，
 * mapper 依据该控件对应模块的 action_id 决定写入统一状态层的哪一路。
 *
 * 关联机制：每次 UI 重建控件时，页面层调用 register() 把
 *   控件根对象 -> 绑定信息
 * 登记到内部表。控件在事件回调里用自身的任一子/根对象调用 find() 向上回溯，
 * 取到绑定后调用 report_*()。这样控件文件无需知道 module/UI 结构体细节，
 * 也不依赖具体输出协议。
 */

typedef struct {
    gamepad_action_id_t action_id;
    gamepad_module_kind_t kind;
    gamepad_preset_id_t preset_id;
    int16_t deadzone;     /* 死区百分比 0..90 */
    int16_t sensitivity;  /* 灵敏度百分比，100 = 1.0 倍 */
} gamepad_action_binding_t;

/* 复合按钮模块的 channel 约定：
 *  - 单按钮：始终用 PRIMARY(0)
 *  - 肩键对：0 = 上键(L1/R1)，1 = 扳机键(L2/R2)
 *  - DPad：直接用 gamepad_dpad_dir_t (UP/DOWN/LEFT/RIGHT) 作为 channel */
#define GAMEPAD_CHANNEL_PRIMARY 0
#define GAMEPAD_CHANNEL_TRIGGER 1

/* 清空绑定表(在页面每次重建控件前调用)。 */
void gamepad_action_mapper_reset(void);

/* 登记 控件根对象 -> 模块绑定。root 为空或表满则忽略。 */
void gamepad_action_mapper_register(lv_obj_t * root, const gamepad_module_instance_t * module);

/* 从任意子对象向上回溯，找到已登记的绑定；找不到返回 NULL。 */
const gamepad_action_binding_t * gamepad_action_mapper_find(lv_obj_t * child);

/* 上报模拟输入(nx/ny ∈ [-1,1]，右为+/上为+)。mapper 依 action 应用死区/灵敏度后写状态。 */
void gamepad_action_mapper_report_analog(const gamepad_action_binding_t * binding, float nx, float ny);

/* 上报离散输入(某 channel 的按下态)。 */
void gamepad_action_mapper_report_button(const gamepad_action_binding_t * binding, uint8_t channel, bool pressed);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_ACTION_MAPPER_H */
