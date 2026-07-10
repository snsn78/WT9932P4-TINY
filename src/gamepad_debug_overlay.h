#ifndef GAMEPAD_DEBUG_OVERLAY_H
#define GAMEPAD_DEBUG_OVERLAY_H

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 屏幕调试观察层 (Step 8)
 * ------------------------------------------------------------------
 * 在页面上显示统一输入状态层的实时内容(move/look/按钮/DPad)，用于联调时
 * 判断问题出在控件、状态层还是映射层。仅只读展示，不参与正式输出，
 * 且自身不可点击，不干扰控件触摸与编辑操作。
 */

/* 在 parent 上创建观察区(含内部定时刷新)，返回根标签对象。重复调用会先销毁旧的。 */
lv_obj_t * gamepad_debug_overlay_create(lv_obj_t * parent);

/* 显示/隐藏观察区。 */
void gamepad_debug_overlay_set_hidden(bool hidden);

/* 销毁观察区并停止定时器(页面重建前调用)。 */
void gamepad_debug_overlay_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_DEBUG_OVERLAY_H */
