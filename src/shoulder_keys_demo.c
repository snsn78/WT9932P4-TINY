#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"

#include <stdbool.h>
#include <stdio.h>

#define SHOULDER_BRIDGE_WIDTH         292
#define SHOULDER_BRIDGE_HEIGHT        214
#define SHOULDER_BRIDGE_TOP            96
#define SHOULDER_SIDE_PANEL_WIDTH     132
#define SHOULDER_SIDE_PANEL_HEIGHT    176
#define SHOULDER_TOP_KEY_WIDTH        108
#define SHOULDER_TOP_KEY_HEIGHT        42
#define SHOULDER_TRIGGER_WIDTH         88
#define SHOULDER_TRIGGER_HEIGHT       104
#define SHOULDER_TOP_KEY_OFFSET_Y      8
#define SHOULDER_TRIGGER_OFFSET_Y      40
#define SHOULDER_BASE_SCALE           256
#define SHOULDER_PRESS_SCALE          236
#define SHOULDER_PRESS_DEPTH            5
#define SHOULDER_IDLE_MS              920
#define SHOULDER_PRESS_MS             110
#define SHOULDER_RELEASE_MS           160
#define SHOULDER_COMPACT_HEIGHT       196

typedef struct shoulder_panel_demo_t shoulder_panel_demo_t;

typedef struct {
    const char * name;
    lv_color_t base_color;
    lv_color_t accent_color;
    lv_color_t active_color;
    bool is_trigger;
    lv_coord_t idle_shadow_min;
    lv_coord_t idle_shadow_max;
    lv_obj_t * button;
    lv_obj_t * cap;
    shoulder_panel_demo_t * owner;
} shoulder_key_t;

struct shoulder_panel_demo_t {
    shoulder_key_t keys[2];
    lv_obj_t * panel;
    lv_obj_t * status_label;
    float scale;
};

static void shoulder_set_status(shoulder_panel_demo_t * demo, const char * text)
{
    if(demo == NULL || demo->status_label == NULL) return;
    lv_label_set_text_fmt(demo->status_label, "Input: %s", text);
}

static void shoulder_shadow_anim_cb(void * var, int32_t value)
{
    shoulder_key_t * key = (shoulder_key_t *)var;
    lv_obj_set_style_shadow_width(key->button, value, 0);
    lv_obj_set_style_shadow_opa(key->button, (lv_opa_t)LV_MIN(220, 92 + value * 3), 0);
}

static void shoulder_scale_anim_cb(void * var, int32_t value)
{
    shoulder_key_t * key = (shoulder_key_t *)var;
    lv_obj_set_style_transform_scale(key->button, value, 0);
}

static void shoulder_depth_anim_cb(void * var, int32_t value)
{
    shoulder_key_t * key = (shoulder_key_t *)var;
    lv_obj_set_style_translate_y(key->button, value, 0);
}

static void shoulder_color_anim_cb(void * var, int32_t value)
{
    shoulder_key_t * key = (shoulder_key_t *)var;
    lv_color_t body_color = lv_color_mix(key->active_color, key->base_color, (uint8_t)value);
    lv_color_t cap_color = lv_color_mix(key->active_color, key->accent_color, (uint8_t)value);
    lv_color_t border_color = lv_color_mix(lv_color_white(), key->active_color, (uint8_t)LV_MIN(255, 72 + value));

    lv_obj_set_style_bg_color(key->button, body_color, 0);
    lv_obj_set_style_bg_grad_color(key->button, cap_color, 0);
    lv_obj_set_style_border_color(key->button, border_color, 0);
    lv_obj_set_style_shadow_color(key->button, body_color, 0);
    lv_obj_set_style_bg_color(key->cap, cap_color, 0);
}

static void shoulder_start_idle_pulse(shoulder_key_t * key)
{
    lv_anim_t anim;

    lv_anim_delete(key, shoulder_shadow_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, shoulder_shadow_anim_cb);
    lv_anim_set_values(&anim, key->idle_shadow_min, key->idle_shadow_max);
    lv_anim_set_duration(&anim, SHOULDER_IDLE_MS);
#if defined(LVGL_VERSION_MINOR) && (LVGL_VERSION_MINOR < 5)
    lv_anim_set_playback_duration(&anim, SHOULDER_IDLE_MS);
#else
    lv_anim_set_reverse_duration(&anim, SHOULDER_IDLE_MS);
#endif
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

static void shoulder_start_scale_anim(shoulder_key_t * key, int32_t start, int32_t end, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(key, shoulder_scale_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, shoulder_scale_anim_cb);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void shoulder_start_depth_anim(shoulder_key_t * key, int32_t start, int32_t end, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(key, shoulder_depth_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, shoulder_depth_anim_cb);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void shoulder_start_color_anim(shoulder_key_t * key, int32_t start, int32_t end, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(key, shoulder_color_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, shoulder_color_anim_cb);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void shoulder_key_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    shoulder_key_t * key = (shoulder_key_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        lv_anim_delete(key, shoulder_shadow_anim_cb);
        shoulder_start_scale_anim(key, SHOULDER_BASE_SCALE, SHOULDER_PRESS_SCALE, SHOULDER_PRESS_MS);
        shoulder_start_depth_anim(key, 0, LV_MAX(2, (int32_t)(SHOULDER_PRESS_DEPTH * key->owner->scale)), SHOULDER_PRESS_MS);
        shoulder_start_color_anim(key, 0, 255, SHOULDER_PRESS_MS + 30U);
        shoulder_set_status(key->owner, key->name);
        {
            /* channel: 上键(is_trigger=false)=L1/R1，扳机键=L2/R2 */
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(key->button);
            const uint8_t channel = key->is_trigger ? GAMEPAD_CHANNEL_TRIGGER : GAMEPAD_CHANNEL_PRIMARY;
            if(binding != NULL) gamepad_action_mapper_report_button(binding, channel, true);
        }
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        shoulder_start_scale_anim(key, SHOULDER_PRESS_SCALE, SHOULDER_BASE_SCALE, SHOULDER_RELEASE_MS);
        shoulder_start_depth_anim(key, LV_MAX(2, (int32_t)(SHOULDER_PRESS_DEPTH * key->owner->scale)), 0, SHOULDER_RELEASE_MS);
        shoulder_start_color_anim(key, 255, 0, SHOULDER_RELEASE_MS + 20U);
        shoulder_start_idle_pulse(key);
        {
            /* PRESS_LOST 也须清零 */
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(key->button);
            const uint8_t channel = key->is_trigger ? GAMEPAD_CHANNEL_TRIGGER : GAMEPAD_CHANNEL_PRIMARY;
            if(binding != NULL) gamepad_action_mapper_report_button(binding, channel, false);
        }
    }
    else if(code == LV_EVENT_CLICKED) {
#if defined(GAMEPAD_INPUT_DEBUG_LOG)
        printf("%s clicked\n", key->name);
        fflush(stdout);
#endif
    }
}

static void shoulder_setup_defaults(shoulder_panel_demo_t * demo, bool left_side)
{
    shoulder_key_t * top = &demo->keys[0];
    shoulder_key_t * trigger = &demo->keys[1];

    if(left_side) {
        top->name = "L1";
        top->base_color = lv_color_hex(0x3274C8);
        top->accent_color = lv_color_hex(0x6CAEF8);
        top->active_color = lv_color_hex(0x92D2FF);
        top->is_trigger = false;
        top->idle_shadow_min = 12;
        top->idle_shadow_max = 20;

        trigger->name = "L2";
        trigger->base_color = lv_color_hex(0x1C8A79);
        trigger->accent_color = lv_color_hex(0x55C7B2);
        trigger->active_color = lv_color_hex(0x8FE9D7);
        trigger->is_trigger = true;
        trigger->idle_shadow_min = 14;
        trigger->idle_shadow_max = 24;
    }
    else {
        top->name = "R1";
        top->base_color = lv_color_hex(0xBE6A1B);
        top->accent_color = lv_color_hex(0xF0A14A);
        top->active_color = lv_color_hex(0xFFD28A);
        top->is_trigger = false;
        top->idle_shadow_min = 12;
        top->idle_shadow_max = 20;

        trigger->name = "R2";
        trigger->base_color = lv_color_hex(0xA34858);
        trigger->accent_color = lv_color_hex(0xD77484);
        trigger->active_color = lv_color_hex(0xFFB2BE);
        trigger->is_trigger = true;
        trigger->idle_shadow_min = 14;
        trigger->idle_shadow_max = 24;
    }

    top->owner = demo;
    trigger->owner = demo;
}

static void shoulder_create_key(lv_obj_t * parent, shoulder_key_t * key, float scale, lv_coord_t y_offset)
{
    lv_obj_t * label;
    lv_coord_t key_width = key->is_trigger ? LV_MAX(64, (int32_t)(SHOULDER_TRIGGER_WIDTH * scale))
                                           : LV_MAX(78, (int32_t)(SHOULDER_TOP_KEY_WIDTH * scale));
    lv_coord_t key_height = key->is_trigger ? LV_MAX(72, (int32_t)(SHOULDER_TRIGGER_HEIGHT * scale))
                                            : LV_MAX(32, (int32_t)(SHOULDER_TOP_KEY_HEIGHT * scale));

    key->button = lv_button_create(parent);
    lv_obj_set_size(key->button, key_width, key_height);
    lv_obj_align(key->button, LV_ALIGN_TOP_MID, 0, y_offset);
    lv_obj_clear_flag(key->button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(key->button, key->is_trigger ? LV_MAX(20, key_height / 3) : LV_MAX(18, key_height / 2), 0);
    lv_obj_set_style_border_width(key->button, 2, 0);
    lv_obj_set_style_border_color(key->button, lv_color_hex(0xEDF5FF), 0);
    lv_obj_set_style_bg_color(key->button, key->base_color, 0);
    lv_obj_set_style_bg_grad_color(key->button, key->accent_color, 0);
    lv_obj_set_style_bg_grad_dir(key->button, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(key->button, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(key->button, key->idle_shadow_min, 0);
    lv_obj_set_style_shadow_opa(key->button, 140, 0);
    lv_obj_set_style_shadow_color(key->button, key->base_color, 0);
    lv_obj_set_style_transform_pivot_x(key->button, key_width / 2, 0);
    lv_obj_set_style_transform_pivot_y(key->button, key_height / 2, 0);
    lv_obj_set_style_transform_scale(key->button, SHOULDER_BASE_SCALE, 0);
    lv_obj_set_style_translate_y(key->button, 0, 0);

    key->cap = lv_obj_create(key->button);
    lv_obj_set_size(key->cap, key_width - LV_MAX(16, (int32_t)(18 * scale)), key->is_trigger ? LV_MAX(12, (int32_t)(16 * scale))
                                                                                               : LV_MAX(8, (int32_t)(10 * scale)));
    lv_obj_align(key->cap, LV_ALIGN_TOP_MID, 0, key->is_trigger ? LV_MAX(6, (int32_t)(10 * scale)) : LV_MAX(5, (int32_t)(7 * scale)));
    lv_obj_clear_flag(key->cap, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(key->cap, LV_MAX(8, lv_obj_get_height(key->cap) / 2), 0);
    lv_obj_set_style_border_width(key->cap, 0, 0);
    lv_obj_set_style_bg_color(key->cap, key->accent_color, 0);
    lv_obj_set_style_bg_opa(key->cap, 110, 0);

    label = lv_label_create(key->button);
    lv_label_set_text(label, key->name);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    lv_obj_align(label, key->is_trigger ? LV_ALIGN_BOTTOM_MID : LV_ALIGN_CENTER, 0, key->is_trigger ? -LV_MAX(8, (int32_t)(12 * scale)) : 0);

    lv_obj_add_event_cb(key->button, shoulder_key_event_cb, LV_EVENT_PRESSED, key);
    lv_obj_add_event_cb(key->button, shoulder_key_event_cb, LV_EVENT_RELEASED, key);
    lv_obj_add_event_cb(key->button, shoulder_key_event_cb, LV_EVENT_PRESS_LOST, key);
    lv_obj_add_event_cb(key->button, shoulder_key_event_cb, LV_EVENT_CLICKED, key);

    shoulder_start_idle_pulse(key);
}

static void shoulder_panel_delete_cb(lv_event_t * e)
{
    shoulder_panel_demo_t * demo = (shoulder_panel_demo_t *)lv_event_get_user_data(e);
    uint32_t i;
    for(i = 0; i < 2U; i++) {
        lv_anim_delete(&demo->keys[i], shoulder_shadow_anim_cb);
        lv_anim_delete(&demo->keys[i], shoulder_scale_anim_cb);
        lv_anim_delete(&demo->keys[i], shoulder_depth_anim_cb);
        lv_anim_delete(&demo->keys[i], shoulder_color_anim_cb);
    }
    lv_free(demo);
}

static lv_obj_t * shoulder_build_side(lv_obj_t * parent,
                                      bool left_side,
                                      lv_coord_t panel_width,
                                      lv_coord_t panel_height,
                                      lv_obj_t * status_label)
{
    shoulder_panel_demo_t * demo = (shoulder_panel_demo_t *)lv_malloc_zeroed(sizeof(*demo));
    lv_coord_t top_offset;
    lv_coord_t trigger_offset;

    if(demo == NULL) return NULL;

    demo->status_label = status_label;
    demo->scale = LV_MIN((float)panel_width / (float)SHOULDER_SIDE_PANEL_WIDTH,
                         (float)panel_height / (float)SHOULDER_SIDE_PANEL_HEIGHT);
    shoulder_setup_defaults(demo, left_side);

    demo->panel = lv_obj_create(parent);
    lv_obj_set_size(demo->panel, panel_width, panel_height);
    lv_obj_clear_flag(demo->panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(demo->panel, LV_MAX(18, (int32_t)(30 * demo->scale)), 0);
    lv_obj_set_style_border_width(demo->panel, 1, 0);
    lv_obj_set_style_border_color(demo->panel, lv_color_hex(0x24415F), 0);
    lv_obj_set_style_bg_color(demo->panel, lv_color_hex(0x11243B), 0);
    lv_obj_set_style_bg_opa(demo->panel, LV_OPA_60, 0);
    lv_obj_add_event_cb(demo->panel, shoulder_panel_delete_cb, LV_EVENT_DELETE, demo);

    top_offset = LV_MAX(4, (int32_t)(SHOULDER_TOP_KEY_OFFSET_Y * demo->scale));
    trigger_offset = LV_MAX(44, (int32_t)(SHOULDER_TRIGGER_OFFSET_Y * demo->scale));

    shoulder_create_key(demo->panel, &demo->keys[0], demo->scale, top_offset);
    shoulder_create_key(demo->panel, &demo->keys[1], demo->scale, trigger_offset);

    return demo->panel;
}

lv_obj_t * shoulder_keys_create_left_compact(lv_obj_t * parent)
{
    return shoulder_build_side(parent, true, SHOULDER_SIDE_PANEL_WIDTH, SHOULDER_SIDE_PANEL_HEIGHT, NULL);
}

lv_obj_t * shoulder_keys_create_left_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    return shoulder_build_side(parent, true, width, height, NULL);
}

lv_obj_t * shoulder_keys_create_right_compact(lv_obj_t * parent)
{
    return shoulder_build_side(parent, false, SHOULDER_SIDE_PANEL_WIDTH, SHOULDER_SIDE_PANEL_HEIGHT, NULL);
}

lv_obj_t * shoulder_keys_create_right_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    return shoulder_build_side(parent, false, width, height, NULL);
}

lv_obj_t * shoulder_keys_create_compact(lv_obj_t * parent)
{
    lv_obj_t * root = lv_obj_create(parent);
    lv_obj_t * left_panel;
    lv_obj_t * right_panel;

    lv_obj_set_size(root, lv_obj_get_content_width(parent), SHOULDER_COMPACT_HEIGHT);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_shadow_width(root, 0, 0);

    left_panel = shoulder_keys_create_left_compact(root);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, 0, 0);

    right_panel = shoulder_keys_create_right_compact(root);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, 0, 0);

    return root;
}

void shoulder_keys_demo_create(void)
{
    lv_obj_t * screen = lv_screen_active();
    lv_obj_t * bridge;
    lv_obj_t * left_panel;
    lv_obj_t * right_panel;
    lv_obj_t * title;
    lv_obj_t * hint;
    lv_obj_t * status_label;

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x08101C), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_hex(0x15263F), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    title = lv_label_create(screen);
    lv_label_set_text(title, "Gamepad Shoulder Keys");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F8FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    hint = lv_label_create(screen);
    lv_label_set_text(hint, "Symmetrical L1/L2 and R1/R2 layout with press animation");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x9EB8D8), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, 280);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 42);

    bridge = lv_obj_create(screen);
    lv_obj_set_size(bridge, SHOULDER_BRIDGE_WIDTH, SHOULDER_BRIDGE_HEIGHT);
    lv_obj_align(bridge, LV_ALIGN_TOP_MID, 0, SHOULDER_BRIDGE_TOP);
    lv_obj_clear_flag(bridge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bridge, 42, 0);
    lv_obj_set_style_border_width(bridge, 1, 0);
    lv_obj_set_style_border_color(bridge, lv_color_hex(0x365A89), 0);
    lv_obj_set_style_bg_color(bridge, lv_color_hex(0x0E1A2E), 0);
    lv_obj_set_style_bg_grad_color(bridge, lv_color_hex(0x13253E), 0);
    lv_obj_set_style_bg_grad_dir(bridge, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(bridge, 30, 0);
    lv_obj_set_style_shadow_color(bridge, lv_color_hex(0x02060E), 0);
    lv_obj_set_style_shadow_opa(bridge, LV_OPA_30, 0);

    status_label = lv_label_create(screen);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xD9E8FF), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(status_label, 240);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_label_set_text(status_label, "Input: Press L1 / L2 / R1 / R2");

    left_panel = shoulder_build_side(bridge, true, SHOULDER_SIDE_PANEL_WIDTH, SHOULDER_SIDE_PANEL_HEIGHT, status_label);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, -15, 16);

    right_panel = shoulder_build_side(bridge, false, SHOULDER_SIDE_PANEL_WIDTH, SHOULDER_SIDE_PANEL_HEIGHT, status_label);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, 30, 16);
}

#if defined(SHOULDER_KEYS_DEMO_STANDALONE)

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
    shoulder_keys_demo_create();

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
