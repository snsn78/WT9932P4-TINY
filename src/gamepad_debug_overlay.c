#include "gamepad_debug_overlay.h"

#include "gamepad_input_state.h"

/* 屏幕调试观察层实现 (Step 8) */

#define DEBUG_OVERLAY_REFRESH_MS 100

static lv_obj_t * g_overlay_label = NULL;
static lv_timer_t * g_overlay_timer = NULL;
static uint32_t g_last_seq = 0xFFFFFFFFu;

static void overlay_refresh(void)
{
    const gamepad_input_state_t * st = gamepad_input_get_state();

    if(g_overlay_label == NULL || st == NULL) return;

    /* 仅在状态实际变化时刷新，减少无谓重绘。 */
    if(st->update_seq == g_last_seq) return;
    g_last_seq = st->update_seq;

    /* 用整型百分比显示，避免依赖 LVGL 的 %f 支持(LV_SPRINTF_USE_FLOAT)。 */
    lv_label_set_text_fmt(g_overlay_label,
                          "move %+4d,%+4d  look %+4d,%+4d\n"
                          "A%d B%d X%d Y%d  L1%d L2%d R1%d R2%d  ST%d SE%d\n"
                          "DPAD U%d D%d L%d R%d",
                          (int)(st->move_x * 100.0f), (int)(st->move_y * 100.0f),
                          (int)(st->look_x * 100.0f), (int)(st->look_y * 100.0f),
                          st->btn_a, st->btn_b, st->btn_x, st->btn_y,
                          st->btn_l1, st->btn_l2, st->btn_r1, st->btn_r2,
                          st->btn_start, st->btn_select,
                          st->dpad_up, st->dpad_down, st->dpad_left, st->dpad_right);
}

static void overlay_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    overlay_refresh();
}

lv_obj_t * gamepad_debug_overlay_create(lv_obj_t * parent)
{
    gamepad_debug_overlay_destroy();

    g_overlay_label = lv_label_create(parent);
    /* 标签默认不可点击，不会拦截控件触摸/编辑操作。 */
    lv_obj_set_style_bg_color(g_overlay_label, lv_color_hex(0x030A14), 0);
    lv_obj_set_style_bg_opa(g_overlay_label, LV_OPA_70, 0);
    lv_obj_set_style_text_color(g_overlay_label, lv_color_hex(0x7DD3FC), 0);
    lv_obj_set_style_radius(g_overlay_label, 8, 0);
    lv_obj_set_style_pad_all(g_overlay_label, 6, 0);
    lv_obj_set_style_border_width(g_overlay_label, 1, 0);
    lv_obj_set_style_border_color(g_overlay_label, lv_color_hex(0x244763), 0);
    lv_obj_align(g_overlay_label, LV_ALIGN_BOTTOM_LEFT, 12, -40);
    lv_obj_move_foreground(g_overlay_label);

    g_last_seq = 0xFFFFFFFFu;
    overlay_refresh();

    g_overlay_timer = lv_timer_create(overlay_timer_cb, DEBUG_OVERLAY_REFRESH_MS, NULL);

    return g_overlay_label;
}

void gamepad_debug_overlay_set_hidden(bool hidden)
{
    if(g_overlay_label == NULL) return;

    if(hidden) {
        lv_obj_add_flag(g_overlay_label, LV_OBJ_FLAG_HIDDEN);
        if(g_overlay_timer != NULL) lv_timer_pause(g_overlay_timer);
    }
    else {
        lv_obj_clear_flag(g_overlay_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_overlay_label);
        if(g_overlay_timer != NULL) lv_timer_resume(g_overlay_timer);
    }
}

void gamepad_debug_overlay_destroy(void)
{
    if(g_overlay_timer != NULL) {
        lv_timer_delete(g_overlay_timer);
        g_overlay_timer = NULL;
    }
    if(g_overlay_label != NULL) {
        lv_obj_delete(g_overlay_label);
        g_overlay_label = NULL;
    }
    g_last_seq = 0xFFFFFFFFu;
}
