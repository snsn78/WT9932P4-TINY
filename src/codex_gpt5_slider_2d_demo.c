/* Author: Codex (GPT-5)
 * File: codex_gpt5_slider_2d_demo.c
 * Purpose: LVGL 9.5 2D view slider with standalone and compact modes
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SLIDER_FULL_PAD_SIZE      180
#define SLIDER_FULL_PUCK_SIZE      52
#define SLIDER_COMPACT_PAD_SIZE   220
#define SLIDER_COMPACT_PUCK_SIZE   64
#define SLIDER_COMPACT_ROOT_W     300
#define SLIDER_COMPACT_ROOT_H     340
#define SLIDER_FULL_ROOT_W        320
#define SLIDER_FULL_ROOT_H        320
#define SLIDER_ANIM_PROGRESS     1024

typedef struct {
    lv_obj_t * root;
    lv_obj_t * pad;
    lv_obj_t * puck_slider;
    lv_obj_t * info_label;
    lv_obj_t * damping_value_label;
    lv_obj_t * speed_value_label;
    float damping;
    uint32_t speed_ms;
    lv_point_t last_vect;
    int32_t current_x;
    int32_t current_y;
    int32_t anim_start_x;
    int32_t anim_start_y;
    int32_t anim_end_x;
    int32_t anim_end_y;
    int32_t pad_size;
    int32_t puck_size;
    int32_t last_report_x_percent;
    int32_t last_report_y_percent;
    int32_t last_report_strength;
    bool compact_mode;
} slider_2d_demo_t;

static int32_t clamp_i32(int32_t value, int32_t min, int32_t max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static float clamp_f32(float value, float min, float max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static int32_t pad_limit(const slider_2d_demo_t * demo)
{
    return demo->pad_size - demo->puck_size;
}

static void update_status(slider_2d_demo_t * demo)
{
    const int32_t center = pad_limit(demo) / 2;
    const int32_t x_percent = (demo->current_x - center) * 100 / LV_MAX(center, 1);
    const int32_t y_percent = (center - demo->current_y) * 100 / LV_MAX(center, 1);
    const float dx = (float)(demo->current_x - center);
    const float dy = (float)(demo->current_y - center);
    const float radius = sqrtf(dx * dx + dy * dy);
    const float max_radius = sqrtf((float)(center * center * 2));
    const int32_t strength = clamp_i32((int32_t)(radius / LV_MAX(max_radius, 1.0f) * 100.0f + 0.5f), 0, 100);

    lv_slider_set_value(demo->puck_slider, strength, LV_ANIM_OFF);

    if(demo->info_label != NULL) {
        if(demo->compact_mode) {
            lv_label_set_text_fmt(demo->info_label, "View X:%+03d  Y:%+03d\nStrength %d%%", x_percent, y_percent, strength);
        }
        else {
            /* 用整型十分位显示阻尼，避免依赖 LVGL 的 %f 支持(LV_USE_FLOAT) */
            lv_label_set_text_fmt(demo->info_label,
                                  "X:%+03d  Y:%+03d\nStrength:%d%%\nDamping:%d.%d  Speed:%lu ms",
                                  x_percent,
                                  y_percent,
                                  strength,
                                  (int)demo->damping,
                                  (int)(demo->damping * 10.0f) % 10,
                                  (unsigned long)demo->speed_ms);
        }
    }

    if(demo->last_report_x_percent != x_percent ||
       demo->last_report_y_percent != y_percent ||
       demo->last_report_strength != strength) {
#if defined(GAMEPAD_INPUT_DEBUG_LOG)
        printf("VIEW_SLIDER x=%+03ld y=%+03ld strength=%ld%%\n",
               (long)x_percent,
               (long)y_percent,
               (long)strength);
        fflush(stdout);
#endif
        /* 正式出口：经动作映射层写入统一状态层(默认绑定 LOOK)。
         * x_percent/y_percent 已是 [-100,100]、右为+/上为+，除以 100 归一化。 */
        {
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(demo->pad);
            if(binding != NULL) {
                gamepad_action_mapper_report_analog(binding,
                                                    (float)x_percent / 100.0f,
                                                    (float)y_percent / 100.0f);
            }
        }
        demo->last_report_x_percent = x_percent;
        demo->last_report_y_percent = y_percent;
        demo->last_report_strength = strength;
    }
}

static void apply_position(slider_2d_demo_t * demo, int32_t x, int32_t y)
{
    const int32_t limit = pad_limit(demo);

    demo->current_x = clamp_i32(x, 0, limit);
    demo->current_y = clamp_i32(y, 0, limit);

    lv_obj_set_pos(demo->puck_slider, demo->current_x, demo->current_y);
    update_status(demo);
}

static void inertia_anim_exec_cb(void * var, int32_t progress)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)var;
    const int32_t dx = demo->anim_end_x - demo->anim_start_x;
    const int32_t dy = demo->anim_end_y - demo->anim_start_y;
    const int32_t x = demo->anim_start_x + (dx * progress) / SLIDER_ANIM_PROGRESS;
    const int32_t y = demo->anim_start_y + (dy * progress) / SLIDER_ANIM_PROGRESS;

    apply_position(demo, x, y);
}

static int32_t damping_anim_path_cb(const lv_anim_t * anim)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_anim_get_user_data(anim);
    const float damping = demo ? demo->damping : 0.6f;
    const float duration = anim->duration > 0 ? (float)anim->duration : 1.0f;
    const float t = clamp_f32((float)anim->act_time / duration, 0.0f, 1.0f);
    const float exponent = 1.2f + (1.0f - damping) * 3.5f;
    const float eased = 1.0f - powf(1.0f - t, exponent);

    return anim->start_value +
           (int32_t)(((anim->end_value - anim->start_value) * eased) + 0.5f);
}

static void start_inertia(slider_2d_demo_t * demo)
{
    /* 视角滑块绑定 LOOK，语义等同右摇杆：松手后阻尼回中，look 归零，
     * 与左摇杆(joystick_return_to_center)对称，避免松手后视角持续偏转(analog 卡键)。 */
    const int32_t center = pad_limit(demo) / 2;
    lv_anim_t anim;

    demo->anim_start_x = demo->current_x;
    demo->anim_start_y = demo->current_y;
    demo->anim_end_x = center;
    demo->anim_end_y = center;

    if(demo->anim_start_x == center && demo->anim_start_y == center) {
        apply_position(demo, center, center);   /* 已在中心，直接上报 0,0 */
        return;
    }

    lv_anim_init(&anim);
    lv_anim_set_var(&anim, demo);
    lv_anim_set_user_data(&anim, demo);
    lv_anim_set_exec_cb(&anim, inertia_anim_exec_cb);
    lv_anim_set_values(&anim, 0, SLIDER_ANIM_PROGRESS);
    lv_anim_set_duration(&anim, demo->speed_ms);
    lv_anim_set_path_cb(&anim, damping_anim_path_cb);
    lv_anim_start(&anim);
}

static void puck_drag_event_cb(lv_event_t * e)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_event_get_user_data(e);
    const lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        demo->last_vect.x = 0;
        demo->last_vect.y = 0;
        lv_anim_delete(demo, inertia_anim_exec_cb);
        return;
    }

    if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_area_t pad_coords;
        lv_point_t point;

        if(indev == NULL) {
            return;
        }

        lv_indev_get_point(indev, &point);
        lv_indev_get_vect(indev, &demo->last_vect);
        lv_obj_get_coords(demo->pad, &pad_coords);

        apply_position(demo,
                       point.x - pad_coords.x1 - demo->puck_size / 2,
                       point.y - pad_coords.y1 - demo->puck_size / 2);
        return;
    }

    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        start_inertia(demo);
    }
}

static void damping_slider_event_cb(lv_event_t * e)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_event_get_user_data(e);
    lv_obj_t * slider = lv_event_get_target_obj(e);

    demo->damping = lv_slider_get_value(slider) / 10.0f;
    if(demo->damping_value_label != NULL) {
        lv_label_set_text_fmt(demo->damping_value_label, "%d.%d",
                              (int)demo->damping, (int)(demo->damping * 10.0f) % 10);
    }
    update_status(demo);
}

static void speed_slider_event_cb(lv_event_t * e)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_event_get_user_data(e);
    lv_obj_t * slider = lv_event_get_target_obj(e);

    demo->speed_ms = (uint32_t)lv_slider_get_value(slider);
    if(demo->speed_value_label != NULL) {
        lv_label_set_text_fmt(demo->speed_value_label, "%lu ms", (unsigned long)demo->speed_ms);
    }
    update_status(demo);
}

static void slider_2d_delete_cb(lv_event_t * e)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_event_get_user_data(e);
    lv_anim_delete(demo, inertia_anim_exec_cb);
    lv_free(demo);
}

static lv_obj_t * slider_2d_build(lv_obj_t * parent, lv_coord_t root_width, lv_coord_t root_height, bool compact_mode)
{
    slider_2d_demo_t * demo = (slider_2d_demo_t *)lv_malloc_zeroed(sizeof(*demo));
    lv_obj_t * title;
    lv_obj_t * cross_h;
    lv_obj_t * cross_v;
    lv_obj_t * control_panel;
    lv_obj_t * damping_title;
    lv_obj_t * damping_slider;
    lv_obj_t * speed_title;
    lv_obj_t * speed_slider;
    float scale_w;
    float scale_h;
    float scale;

    if(demo == NULL) return NULL;

    demo->compact_mode = compact_mode;
    demo->damping = 0.6f;
    demo->speed_ms = 400;
    demo->last_report_x_percent = INT32_MIN;
    demo->last_report_y_percent = INT32_MIN;
    demo->last_report_strength = INT32_MIN;
    scale_w = (float)root_width / (float)(compact_mode ? SLIDER_COMPACT_ROOT_W : SLIDER_FULL_ROOT_W);
    scale_h = (float)root_height / (float)(compact_mode ? SLIDER_COMPACT_ROOT_H : SLIDER_FULL_ROOT_H);
    scale = LV_MIN(scale_w, scale_h);
    demo->pad_size = compact_mode ? LV_MAX(150, (int32_t)(SLIDER_COMPACT_PAD_SIZE * scale))
                                  : LV_MAX(140, (int32_t)(SLIDER_FULL_PAD_SIZE * scale));
    demo->puck_size = compact_mode ? LV_MAX(44, (int32_t)(SLIDER_COMPACT_PUCK_SIZE * scale))
                                   : LV_MAX(38, (int32_t)(SLIDER_FULL_PUCK_SIZE * scale));

    demo->root = lv_obj_create(parent);
    lv_obj_set_user_data(demo->root, demo);
    lv_obj_clear_flag(demo->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(demo->root, 28, 0);
    lv_obj_set_style_border_width(demo->root, compact_mode ? 1 : 0, 0);
    lv_obj_set_style_border_color(demo->root, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(demo->root, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(demo->root, compact_mode ? LV_OPA_80 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(demo->root, compact_mode ? 18 : 0, 0);
    lv_obj_set_style_shadow_color(demo->root, lv_color_hex(0x020617), 0);
    lv_obj_set_style_pad_all(demo->root, LV_MAX(10, (int32_t)(16 * scale)), 0);
    lv_obj_set_size(demo->root, root_width, root_height);
    lv_obj_add_event_cb(demo->root, slider_2d_delete_cb, LV_EVENT_DELETE, demo);

    if(!compact_mode) {
        title = lv_label_create(demo->root);
        lv_label_set_text(title, "LVGL 9.5 Joystick Slider Demo");
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    }

    demo->pad = lv_obj_create(demo->root);
    lv_obj_set_size(demo->pad, demo->pad_size, demo->pad_size);
    lv_obj_align(demo->pad, LV_ALIGN_TOP_MID, 0, compact_mode ? LV_MAX(2, (int32_t)(4 * scale)) : LV_MAX(18, (int32_t)(34 * scale)));
    lv_obj_set_style_radius(demo->pad, LV_MAX(18, (int32_t)(24 * scale)), 0);
    lv_obj_set_style_bg_color(demo->pad, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_grad_color(demo->pad, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_grad_dir(demo->pad, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(demo->pad, 2, 0);
    lv_obj_set_style_border_color(demo->pad, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_shadow_width(demo->pad, LV_MAX(14, (int32_t)(20 * scale)), 0);
    lv_obj_set_style_shadow_color(demo->pad, lv_color_hex(0x0EA5E9), 0);
    lv_obj_set_style_pad_all(demo->pad, 0, 0);
    lv_obj_clear_flag(demo->pad, LV_OBJ_FLAG_SCROLLABLE);

    cross_h = lv_obj_create(demo->pad);
    lv_obj_remove_style_all(cross_h);
    lv_obj_set_size(cross_h, demo->pad_size - LV_MAX(40, (int32_t)(60 * scale)), 2);
    lv_obj_set_style_bg_color(cross_h, lv_color_hex(0x334155), 0);
    lv_obj_center(cross_h);

    cross_v = lv_obj_create(demo->pad);
    lv_obj_remove_style_all(cross_v);
    lv_obj_set_size(cross_v, 2, demo->pad_size - LV_MAX(40, (int32_t)(60 * scale)));
    lv_obj_set_style_bg_color(cross_v, lv_color_hex(0x334155), 0);
    lv_obj_center(cross_v);

    demo->puck_slider = lv_slider_create(demo->pad);
    lv_obj_set_size(demo->puck_slider, demo->puck_size, demo->puck_size);
    lv_slider_set_range(demo->puck_slider, 0, 100);
    lv_obj_add_flag(demo->puck_slider, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_set_ext_click_area(demo->puck_slider, 8);
    lv_obj_set_style_bg_opa(demo->puck_slider, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(demo->puck_slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(demo->puck_slider, lv_color_hex(0x38BDF8), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_color(demo->puck_slider, lv_color_hex(0x22D3EE), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(demo->puck_slider, LV_GRAD_DIR_VER, LV_PART_KNOB);
    lv_obj_set_style_radius(demo->puck_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(demo->puck_slider, LV_MAX(12, (int32_t)(24 * scale)), LV_PART_KNOB);
    lv_obj_set_style_shadow_color(demo->puck_slider, lv_color_hex(0x38BDF8), LV_PART_KNOB);
    lv_obj_add_event_cb(demo->puck_slider, puck_drag_event_cb, LV_EVENT_ALL, demo);

    if(!compact_mode) {
        control_panel = lv_obj_create(demo->root);
        lv_obj_set_size(control_panel, LV_MAX(90, (int32_t)(102 * scale)), LV_MAX(150, (int32_t)(180 * scale)));
        lv_obj_align(control_panel, LV_ALIGN_TOP_RIGHT, 0, LV_MAX(18, (int32_t)(34 * scale)));
        lv_obj_set_style_radius(control_panel, LV_MAX(14, (int32_t)(18 * scale)), 0);
        lv_obj_set_style_bg_color(control_panel, lv_color_hex(0x111827), 0);
        lv_obj_set_style_border_width(control_panel, 1, 0);
        lv_obj_set_style_border_color(control_panel, lv_color_hex(0x334155), 0);
        lv_obj_set_style_pad_all(control_panel, LV_MAX(8, (int32_t)(10 * scale)), 0);
        lv_obj_clear_flag(control_panel, LV_OBJ_FLAG_SCROLLABLE);

        damping_title = lv_label_create(control_panel);
        lv_label_set_text(damping_title, "Damping");
        lv_obj_align(damping_title, LV_ALIGN_TOP_LEFT, 0, 0);

        demo->damping_value_label = lv_label_create(control_panel);
        lv_label_set_text(demo->damping_value_label, "0.6");
        lv_obj_align(demo->damping_value_label, LV_ALIGN_TOP_RIGHT, 0, 0);

        damping_slider = lv_slider_create(control_panel);
        lv_obj_set_width(damping_slider, LV_MAX(64, (int32_t)(80 * scale)));
        lv_obj_align(damping_slider, LV_ALIGN_TOP_MID, 0, LV_MAX(20, (int32_t)(30 * scale)));
        lv_slider_set_range(damping_slider, 1, 10);
        lv_slider_set_value(damping_slider, 6, LV_ANIM_OFF);
        lv_obj_add_event_cb(damping_slider, damping_slider_event_cb, LV_EVENT_VALUE_CHANGED, demo);

        speed_title = lv_label_create(control_panel);
        lv_label_set_text(speed_title, "Speed");
        lv_obj_align(speed_title, LV_ALIGN_TOP_LEFT, 0, LV_MAX(56, (int32_t)(82 * scale)));

        demo->speed_value_label = lv_label_create(control_panel);
        lv_label_set_text(demo->speed_value_label, "400 ms");
        lv_obj_align(demo->speed_value_label, LV_ALIGN_TOP_RIGHT, 0, LV_MAX(56, (int32_t)(82 * scale)));

        speed_slider = lv_slider_create(control_panel);
        lv_obj_set_width(speed_slider, LV_MAX(64, (int32_t)(80 * scale)));
        lv_obj_align(speed_slider, LV_ALIGN_TOP_MID, 0, LV_MAX(76, (int32_t)(112 * scale)));
        lv_slider_set_range(speed_slider, 100, 1000);
        lv_slider_set_value(speed_slider, 400, LV_ANIM_OFF);
        lv_obj_add_event_cb(speed_slider, speed_slider_event_cb, LV_EVENT_VALUE_CHANGED, demo);
    }

    demo->info_label = lv_label_create(demo->root);
    lv_obj_set_style_text_color(demo->info_label, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_align(demo->info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(demo->info_label, compact_mode ? root_width - 40 : root_width - 30);
    lv_obj_align(demo->info_label, LV_ALIGN_BOTTOM_MID, 0, compact_mode ? -LV_MAX(8, (int32_t)(12 * scale))
                                                                         : -LV_MAX(4, (int32_t)(6 * scale)));
    lv_obj_add_flag(demo->info_label, LV_OBJ_FLAG_HIDDEN);

    apply_position(demo, pad_limit(demo) / 2, pad_limit(demo) / 2);

    return demo->root;
}

lv_obj_t * slider_2d_create_compact(lv_obj_t * parent)
{
    return slider_2d_build(parent, SLIDER_COMPACT_ROOT_W, SLIDER_COMPACT_ROOT_H, true);
}

lv_obj_t * slider_2d_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    return slider_2d_build(parent, width, height, true);
}

void slider_2d_set_multitouch_position(lv_obj_t * root, bool active, lv_coord_t screen_x, lv_coord_t screen_y)
{
    slider_2d_demo_t * demo;
    lv_area_t pad_coords;

    if(root == NULL) return;
    demo = (slider_2d_demo_t *)lv_obj_get_user_data(root);
    if(demo == NULL) return;

    if(!active) {
        start_inertia(demo);
        return;
    }

    lv_anim_delete(demo, inertia_anim_exec_cb);
    lv_obj_get_coords(demo->pad, &pad_coords);
    apply_position(demo,
                   screen_x - pad_coords.x1 - demo->puck_size / 2,
                   screen_y - pad_coords.y1 - demo->puck_size / 2);
}

void slider_2d_demo_create(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_t * root;

    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xF8FAFC), 0);

    root = slider_2d_build(scr, SLIDER_FULL_ROOT_W, SLIDER_FULL_ROOT_H, false);
    lv_obj_center(root);
}

#if defined(SLIDER_2D_DEMO_STANDALONE)

#if defined(_WIN32)
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "../lv_port_pc_vscode-master/src/hal/hal.h"

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    lv_init();
    sdl_hal_init(320, 480);
    slider_2d_demo_create();

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        if(sleep_time_ms == LV_NO_TIMER_READY) {
            sleep_time_ms = LV_DEF_REFR_PERIOD;
        }

#if defined(_WIN32)
        Sleep(sleep_time_ms);
#else
        usleep(sleep_time_ms * 1000);
#endif
    }

    return 0;
}

#endif
