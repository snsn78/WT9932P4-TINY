#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define JOYSTICK_BASE_RADIUS           108
#define JOYSTICK_KNOB_RADIUS            34
#define JOYSTICK_PANEL_WIDTH           288
#define JOYSTICK_PANEL_HEIGHT          430
#define JOYSTICK_COMPACT_PANEL_SIZE    252

typedef struct {
    lv_obj_t * panel;
    lv_obj_t * base;
    lv_obj_t * shaft;
    lv_obj_t * shaft_glow;
    lv_obj_t * knob;
    lv_obj_t * coord_label;
    lv_obj_t * dir_label;
    lv_point_precise_t shaft_points[2];
    int32_t base_radius;
    int32_t knob_radius;
    int32_t limit_radius;
    int32_t knob_x;
    int32_t knob_y;
    int32_t last_report_x;
    int32_t last_report_y;
    char last_report_dir[24];
} joystick_demo_t;

static const char * joystick_get_direction(int32_t x, int32_t y)
{
    const int32_t dead_zone = 12;

    if(abs(x) <= dead_zone && abs(y) <= dead_zone) {
        return "CENTER";
    }

    if(abs(x) <= dead_zone) {
        return y > 0 ? "UP" : "DOWN";
    }

    if(abs(y) <= dead_zone) {
        return x > 0 ? "RIGHT" : "LEFT";
    }

    if(x > 0 && y > 0) return "UP-RIGHT";
    if(x > 0) return "DOWN-RIGHT";
    if(y > 0) return "UP-LEFT";
    return "DOWN-LEFT";
}

static void joystick_refresh_readout(joystick_demo_t * js)
{
    char text[64];
    const int32_t display_y = -js->knob_y;

    if(js->coord_label != NULL) {
        lv_snprintf(text, sizeof(text), "X:%4ld  Y:%4ld", (long)js->knob_x, (long)display_y);
        lv_label_set_text(js->coord_label, text);
    }

    if(js->dir_label != NULL) {
        lv_snprintf(text, sizeof(text), "Direction: %s", joystick_get_direction(js->knob_x, display_y));
        lv_label_set_text(js->dir_label, text);
    }
}

static void joystick_emit_output(joystick_demo_t * js)
{
    const int32_t display_y = -js->knob_y;
    const char * direction = joystick_get_direction(js->knob_x, display_y);

    if(js->last_report_x == js->knob_x &&
       js->last_report_y == display_y &&
       strcmp(js->last_report_dir, direction) == 0) {
        return;
    }

#if defined(GAMEPAD_INPUT_DEBUG_LOG)
    printf("LEFT_STICK x=%ld y=%ld dir=%s\n", (long)js->knob_x, (long)display_y, direction);
    fflush(stdout);
#endif

    /* 正式出口：经动作映射层写入统一状态层(默认绑定 MOVE)。
     * 归一化分母必须用 limit_radius(满偏半径)，而非 base_radius。 */
    {
        const gamepad_action_binding_t * binding = gamepad_action_mapper_find(js->base);
        if(binding != NULL && js->limit_radius > 0) {
            const float nx = (float)js->knob_x / (float)js->limit_radius;
            const float ny = (float)display_y / (float)js->limit_radius;
            gamepad_action_mapper_report_analog(binding, nx, ny);
        }
    }

    js->last_report_x = js->knob_x;
    js->last_report_y = display_y;
    lv_snprintf(js->last_report_dir, sizeof(js->last_report_dir), "%s", direction);
}

static void joystick_apply_vector(joystick_demo_t * js, int32_t x, int32_t y)
{
    const int32_t center = js->base_radius;
    int32_t magnitude = LV_ABS(x) + LV_ABS(y);

    js->knob_x = x;
    js->knob_y = y;

    js->shaft_points[0].x = center;
    js->shaft_points[0].y = center;
    js->shaft_points[1].x = center + x;
    js->shaft_points[1].y = center + y;

    if(magnitude == 0) {
        lv_obj_set_style_opa(js->shaft, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_opa(js->shaft_glow, LV_OPA_TRANSP, LV_PART_MAIN);
    }
    else {
        lv_opa_t shaft_opa = (lv_opa_t)LV_MIN(220, 120 + magnitude / 2);
        lv_obj_set_style_opa(js->shaft, shaft_opa, LV_PART_MAIN);
        lv_obj_set_style_opa(js->shaft_glow, (lv_opa_t)LV_MIN(64, 20 + magnitude / 4), LV_PART_MAIN);
    }

    lv_obj_set_pos(js->knob, center + x - js->knob_radius, center + y - js->knob_radius);

    joystick_refresh_readout(js);
    joystick_emit_output(js);
}

static void joystick_anim_x_cb(void * var, int32_t value)
{
    joystick_demo_t * js = (joystick_demo_t *)var;
    joystick_apply_vector(js, value, js->knob_y);
}

static void joystick_anim_y_cb(void * var, int32_t value)
{
    joystick_demo_t * js = (joystick_demo_t *)var;
    joystick_apply_vector(js, js->knob_x, value);
}

static void joystick_return_to_center(joystick_demo_t * js)
{
    lv_anim_t anim_x;
    lv_anim_t anim_y;

    lv_anim_delete(js, joystick_anim_x_cb);
    lv_anim_delete(js, joystick_anim_y_cb);

    lv_anim_init(&anim_x);
    lv_anim_set_var(&anim_x, js);
    lv_anim_set_exec_cb(&anim_x, joystick_anim_x_cb);
    lv_anim_set_values(&anim_x, js->knob_x, 0);
    lv_anim_set_duration(&anim_x, 220);
    lv_anim_set_path_cb(&anim_x, lv_anim_path_ease_out);
    lv_anim_start(&anim_x);

    lv_anim_init(&anim_y);
    lv_anim_set_var(&anim_y, js);
    lv_anim_set_exec_cb(&anim_y, joystick_anim_y_cb);
    lv_anim_set_values(&anim_y, js->knob_y, 0);
    lv_anim_set_duration(&anim_y, 220);
    lv_anim_set_path_cb(&anim_y, lv_anim_path_ease_out);
    lv_anim_start(&anim_y);
}

static void joystick_track_pointer(joystick_demo_t * js)
{
    lv_indev_t * indev = lv_indev_active();
    lv_area_t coords;
    lv_point_t point;
    int32_t dx;
    int32_t dy;
    int64_t dist_sq;

    if(indev == NULL) return;

    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(js->base, &coords);

    dx = point.x - ((coords.x1 + coords.x2) / 2);
    dy = point.y - ((coords.y1 + coords.y2) / 2);
    dist_sq = (int64_t)dx * dx + (int64_t)dy * dy;

    if(dist_sq > (int64_t)js->limit_radius * js->limit_radius) {
        double scale = (double)js->limit_radius / sqrt((double)dist_sq);
        dx = (int32_t)(dx * scale);
        dy = (int32_t)(dy * scale);
    }

    lv_anim_delete(js, joystick_anim_x_cb);
    lv_anim_delete(js, joystick_anim_y_cb);
    joystick_apply_vector(js, dx, dy);
}

static void joystick_event_cb(lv_event_t * e)
{
    joystick_demo_t * js = (joystick_demo_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        joystick_track_pointer(js);
        return;
    }

    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        joystick_return_to_center(js);
    }
}

static void joystick_panel_delete_cb(lv_event_t * e)
{
    joystick_demo_t * js = (joystick_demo_t *)lv_event_get_user_data(e);
    lv_anim_delete(js, joystick_anim_x_cb);
    lv_anim_delete(js, joystick_anim_y_cb);
    lv_free(js);
}

static lv_obj_t * joystick_stick_build(lv_obj_t * parent,
                                       lv_coord_t panel_width,
                                       lv_coord_t panel_height,
                                       bool compact_mode)
{
    joystick_demo_t * js = (joystick_demo_t *)lv_malloc_zeroed(sizeof(*js));
    lv_obj_t * title;
    lv_obj_t * hint;
    lv_obj_t * base_glow;
    lv_obj_t * cross_h;
    lv_obj_t * cross_v;
    lv_obj_t * center_dot;
    lv_obj_t * knob_shine;
    lv_coord_t base_center_y = compact_mode ? 0 : 26;
    lv_coord_t min_dim;
    lv_coord_t base_diameter;
    lv_coord_t glow_size;
    lv_coord_t cross_len;

    if(js == NULL) return NULL;

    min_dim = LV_MIN(panel_width, panel_height);
    base_diameter = LV_MAX(140, min_dim - 36);
    js->base_radius = base_diameter / 2;
    js->knob_radius = LV_MAX(22, (base_diameter * JOYSTICK_KNOB_RADIUS) / (JOYSTICK_BASE_RADIUS * 2));
    js->limit_radius = js->base_radius - js->knob_radius - 10;
    js->last_report_x = INT32_MIN;
    js->last_report_y = INT32_MIN;
    js->last_report_dir[0] = '\0';
    glow_size = base_diameter + 20;
    cross_len = LV_MAX(64, base_diameter - 76);

    js->panel = lv_obj_create(parent);
    lv_obj_set_size(js->panel, panel_width, panel_height);
    lv_obj_clear_flag(js->panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(js->panel, 28, 0);
    lv_obj_set_style_border_width(js->panel, 1, 0);
    lv_obj_set_style_border_color(js->panel, lv_color_hex(0x365B8C), 0);
    lv_obj_set_style_bg_color(js->panel, lv_color_hex(0x0E1A2D), 0);
    lv_obj_set_style_bg_opa(js->panel, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(js->panel, compact_mode ? 22 : 32, 0);
    lv_obj_set_style_shadow_color(js->panel, lv_color_hex(0x02060D), 0);
    lv_obj_set_style_pad_all(js->panel, compact_mode ? 18 : 16, 0);
    lv_obj_add_event_cb(js->panel, joystick_panel_delete_cb, LV_EVENT_DELETE, js);

    if(!compact_mode) {
        title = lv_label_create(js->panel);
        lv_label_set_text(title, "LVGL Joystick Demo");
        lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FB), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

        js->coord_label = lv_label_create(js->panel);
        lv_obj_set_style_text_color(js->coord_label, lv_color_hex(0x7ED9FF), 0);
        lv_obj_align(js->coord_label, LV_ALIGN_TOP_MID, 0, 36);

        hint = lv_label_create(js->panel);
        lv_label_set_text(hint, "Drag the stick with mouse or touch");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x94A8C6), 0);
        lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 62);
    }

    base_glow = lv_obj_create(js->panel);
    lv_obj_set_size(base_glow, glow_size, glow_size);
    lv_obj_align(base_glow, LV_ALIGN_CENTER, 0, base_center_y);
    lv_obj_clear_flag(base_glow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(base_glow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(base_glow, 0, 0);
    lv_obj_set_style_bg_color(base_glow, lv_color_hex(0x17365A), 0);
    lv_obj_set_style_bg_opa(base_glow, 56, 0);
    lv_obj_set_style_shadow_width(base_glow, 28, 0);
    lv_obj_set_style_shadow_color(base_glow, lv_color_hex(0x163B66), 0);

    js->base = lv_obj_create(js->panel);
    lv_obj_set_size(js->base, base_diameter, base_diameter);
    lv_obj_align(js->base, LV_ALIGN_CENTER, 0, base_center_y);
    lv_obj_add_flag(js->base, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(js->base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(js->base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(js->base, 4, 0);
    lv_obj_set_style_border_color(js->base, lv_color_hex(0x6FCBFF), 0);
    lv_obj_set_style_outline_width(js->base, 1, 0);
    lv_obj_set_style_outline_color(js->base, lv_color_hex(0x2C7DB8), 0);
    lv_obj_set_style_outline_pad(js->base, 4, 0);
    lv_obj_set_style_bg_color(js->base, lv_color_hex(0x112642), 0);
    lv_obj_set_style_bg_opa(js->base, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(js->base, 0, 0);

    cross_h = lv_obj_create(js->base);
    lv_obj_set_size(cross_h, cross_len, 2);
    lv_obj_center(cross_h);
    lv_obj_clear_flag(cross_h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cross_h, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(cross_h, 0, 0);
    lv_obj_set_style_bg_color(cross_h, lv_color_hex(0x365B8C), 0);
    lv_obj_set_style_bg_opa(cross_h, 140, 0);

    cross_v = lv_obj_create(js->base);
    lv_obj_set_size(cross_v, 2, cross_len);
    lv_obj_center(cross_v);
    lv_obj_clear_flag(cross_v, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cross_v, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(cross_v, 0, 0);
    lv_obj_set_style_bg_color(cross_v, lv_color_hex(0x365B8C), 0);
    lv_obj_set_style_bg_opa(cross_v, 140, 0);

    js->shaft = lv_line_create(js->base);
    lv_obj_set_size(js->shaft, base_diameter, base_diameter);
    lv_obj_set_pos(js->shaft, 0, 0);
    lv_obj_clear_flag(js->shaft, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style_all(js->shaft);
    lv_line_set_points_mutable(js->shaft, js->shaft_points, 2);
    lv_obj_set_style_line_width(js->shaft, LV_MAX(8, base_diameter / 14), LV_PART_MAIN);
    lv_obj_set_style_line_color(js->shaft, lv_color_hex(0x4B82A8), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(js->shaft, true, LV_PART_MAIN);
    lv_obj_set_style_opa(js->shaft, LV_OPA_TRANSP, LV_PART_MAIN);

    js->shaft_glow = lv_line_create(js->base);
    lv_obj_set_size(js->shaft_glow, base_diameter, base_diameter);
    lv_obj_set_pos(js->shaft_glow, 0, 0);
    lv_obj_clear_flag(js->shaft_glow, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style_all(js->shaft_glow);
    lv_line_set_points_mutable(js->shaft_glow, js->shaft_points, 2);
    lv_obj_set_style_line_width(js->shaft_glow, LV_MAX(12, base_diameter / 10), LV_PART_MAIN);
    lv_obj_set_style_line_color(js->shaft_glow, lv_color_hex(0x224A6F), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(js->shaft_glow, true, LV_PART_MAIN);
    lv_obj_set_style_opa(js->shaft_glow, LV_OPA_TRANSP, LV_PART_MAIN);

    center_dot = lv_obj_create(js->base);
    lv_obj_set_size(center_dot, LV_MAX(10, base_diameter / 16), LV_MAX(10, base_diameter / 16));
    lv_obj_center(center_dot);
    lv_obj_clear_flag(center_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_color_hex(0x6FCBFF), 0);
    lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);

    js->knob = lv_obj_create(js->base);
    lv_obj_set_size(js->knob, js->knob_radius * 2, js->knob_radius * 2);
    lv_obj_clear_flag(js->knob, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(js->knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(js->knob, 2, 0);
    lv_obj_set_style_border_color(js->knob, lv_color_hex(0xF7FBFF), 0);
    lv_obj_set_style_bg_color(js->knob, lv_color_hex(0x59AFFF), 0);
    lv_obj_set_style_bg_grad_color(js->knob, lv_color_hex(0x2375D7), 0);
    lv_obj_set_style_bg_grad_dir(js->knob, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(js->knob, LV_MAX(12, js->knob_radius / 2), 0);
    lv_obj_set_style_shadow_color(js->knob, lv_color_hex(0x061321), 0);

    knob_shine = lv_obj_create(js->knob);
    lv_obj_set_size(knob_shine, LV_MAX(14, js->knob_radius / 2), LV_MAX(14, js->knob_radius / 2));
    lv_obj_align(knob_shine, LV_ALIGN_TOP_LEFT, LV_MAX(8, js->knob_radius / 3), LV_MAX(6, js->knob_radius / 4));
    lv_obj_clear_flag(knob_shine, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(knob_shine, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(knob_shine, 0, 0);
    lv_obj_set_style_bg_color(knob_shine, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(knob_shine, 96, 0);

    if(!compact_mode) {
        js->dir_label = lv_label_create(js->panel);
        lv_obj_set_style_text_color(js->dir_label, lv_color_hex(0xD7E6FF), 0);
        lv_obj_align(js->dir_label, LV_ALIGN_BOTTOM_MID, 0, -26);
    }

    lv_obj_add_event_cb(js->base, joystick_event_cb, LV_EVENT_ALL, js);
    joystick_apply_vector(js, 0, 0);

    return js->panel;
}

lv_obj_t * joystick_stick_create_compact(lv_obj_t * parent)
{
    return joystick_stick_build(parent, JOYSTICK_COMPACT_PANEL_SIZE, JOYSTICK_COMPACT_PANEL_SIZE, true);
}

lv_obj_t * joystick_stick_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    return joystick_stick_build(parent, width, height, true);
}

void joystick_stick_demo_create(void)
{
    lv_obj_t * screen = lv_screen_active();
    lv_obj_t * panel;

    lv_obj_clean(screen);

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x08111F), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_hex(0x13233D), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    panel = joystick_stick_build(screen, JOYSTICK_PANEL_WIDTH, JOYSTICK_PANEL_HEIGHT, false);
    lv_obj_center(panel);
}

#if defined(JOYSTICK_STICK_DEMO_STANDALONE)

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <SDL.h>
#include "../lv_port_pc_vscode-master/src/hal/hal.h"

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;
    lv_init();
    sdl_hal_init(320, 480);
    joystick_stick_demo_create();

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        if(sleep_time_ms == LV_NO_TIMER_READY) {
            sleep_time_ms = LV_DEF_REFR_PERIOD;
        }

#ifdef _MSC_VER
        Sleep(sleep_time_ms);
#else
        usleep(sleep_time_ms * 1000U);
#endif
    }
}

#endif
