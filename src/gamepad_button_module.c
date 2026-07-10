#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"

#include <stdio.h>
#include <string.h>

#define BUTTON_DEFAULT_W 92
#define BUTTON_DEFAULT_H 92
#define BUTTON_BASE_SCALE LV_SCALE_NONE
#define BUTTON_PRESS_SCALE 228

typedef struct {
    const char * text;
    lv_color_t base_color;
    lv_color_t accent_color;
    lv_color_t active_color;
    lv_obj_t * button;
} gamepad_button_state_t;

static void button_resolve_colors(const char * text,
                                  lv_color_t * base,
                                  lv_color_t * accent,
                                  lv_color_t * active)
{
    if(strcmp(text, "A") == 0) {
        *base = lv_color_hex(0x1FA866);
        *accent = lv_color_hex(0x55D892);
        *active = lv_color_hex(0xA4F7C7);
        return;
    }
    if(strcmp(text, "B") == 0) {
        *base = lv_color_hex(0xC84A52);
        *accent = lv_color_hex(0xEA7D83);
        *active = lv_color_hex(0xFFBBC0);
        return;
    }
    if(strcmp(text, "X") == 0) {
        *base = lv_color_hex(0x2C73D9);
        *accent = lv_color_hex(0x67A6FF);
        *active = lv_color_hex(0xB5D5FF);
        return;
    }
    if(strcmp(text, "Y") == 0) {
        *base = lv_color_hex(0xC59617);
        *accent = lv_color_hex(0xF2C94C);
        *active = lv_color_hex(0xFFE695);
        return;
    }
    *base = lv_color_hex(0x475569);
    *accent = lv_color_hex(0x64748B);
    *active = lv_color_hex(0xCBD5E1);
}

static void button_scale_anim_cb(void * var, int32_t value)
{
    gamepad_button_state_t * state = (gamepad_button_state_t *)var;
    lv_obj_set_style_transform_scale(state->button, value, 0);
}

static void button_color_anim_cb(void * var, int32_t value)
{
    gamepad_button_state_t * state = (gamepad_button_state_t *)var;
    lv_color_t bg = lv_color_mix(state->active_color, state->base_color, (uint8_t)value);
    lv_color_t accent = lv_color_mix(state->active_color, state->accent_color, (uint8_t)value);

    lv_obj_set_style_bg_color(state->button, bg, 0);
    lv_obj_set_style_bg_grad_color(state->button, accent, 0);
    lv_obj_set_style_shadow_color(state->button, bg, 0);
}

static void start_button_scale_anim(gamepad_button_state_t * state, int32_t from, int32_t to, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(state, button_scale_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, state);
    lv_anim_set_exec_cb(&anim, button_scale_anim_cb);
    lv_anim_set_values(&anim, from, to);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void start_button_color_anim(gamepad_button_state_t * state, int32_t from, int32_t to, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(state, button_color_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, state);
    lv_anim_set_exec_cb(&anim, button_color_anim_cb);
    lv_anim_set_values(&anim, from, to);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void gamepad_button_event_cb(lv_event_t * e)
{
    gamepad_button_state_t * state = (gamepad_button_state_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        start_button_scale_anim(state, BUTTON_BASE_SCALE, BUTTON_PRESS_SCALE, 90);
        start_button_color_anim(state, 0, 255, 110);
        {
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(state->button);
            if(binding != NULL) gamepad_action_mapper_report_button(binding, GAMEPAD_CHANNEL_PRIMARY, true);
        }
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        start_button_scale_anim(state, BUTTON_PRESS_SCALE, BUTTON_BASE_SCALE, 140);
        start_button_color_anim(state, 255, 0, 160);
        {
            /* PRESS_LOST 也须清零，避免拖出按钮后卡在按下态 */
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(state->button);
            if(binding != NULL) gamepad_action_mapper_report_button(binding, GAMEPAD_CHANNEL_PRIMARY, false);
        }
    }
    else if(code == LV_EVENT_CLICKED) {
#if defined(GAMEPAD_INPUT_DEBUG_LOG)
        printf("%s pressed\n", state->text);
        fflush(stdout);
#endif
    }
}

static void gamepad_button_delete_cb(lv_event_t * e)
{
    gamepad_button_state_t * state = (gamepad_button_state_t *)lv_event_get_user_data(e);
    lv_anim_delete(state, button_scale_anim_cb);
    lv_anim_delete(state, button_color_anim_cb);
    lv_free(state);
}

lv_obj_t * gamepad_button_create_compact_sized(lv_obj_t * parent,
                                               const char * text,
                                               lv_coord_t width,
                                               lv_coord_t height)
{
    gamepad_button_state_t * state = (gamepad_button_state_t *)lv_malloc_zeroed(sizeof(*state));
    lv_obj_t * root;
    lv_obj_t * label;
    lv_obj_t * shine;

    if(state == NULL) return NULL;

    state->text = text;
    button_resolve_colors(text, &state->base_color, &state->accent_color, &state->active_color);

    root = lv_button_create(parent);
    state->button = root;
    lv_obj_set_size(root, width, height);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(root, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(root, 2, 0);
    lv_obj_set_style_border_color(root, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_color(root, state->base_color, 0);
    lv_obj_set_style_bg_grad_color(root, state->accent_color, 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(root, LV_MAX(14, width / 4), 0);
    lv_obj_set_style_shadow_color(root, state->base_color, 0);
    lv_obj_set_style_shadow_opa(root, LV_OPA_30, 0);
    lv_obj_set_style_transform_pivot_x(root, width / 2, 0);
    lv_obj_set_style_transform_pivot_y(root, height / 2, 0);
    lv_obj_set_style_transform_scale(root, BUTTON_BASE_SCALE, 0);
    lv_obj_add_event_cb(root, gamepad_button_event_cb, LV_EVENT_PRESSED, state);
    lv_obj_add_event_cb(root, gamepad_button_event_cb, LV_EVENT_RELEASED, state);
    lv_obj_add_event_cb(root, gamepad_button_event_cb, LV_EVENT_PRESS_LOST, state);
    lv_obj_add_event_cb(root, gamepad_button_event_cb, LV_EVENT_CLICKED, state);
    lv_obj_add_event_cb(root, gamepad_button_delete_cb, LV_EVENT_DELETE, state);

    shine = lv_obj_create(root);
    lv_obj_set_size(shine, LV_MAX(14, width / 4), LV_MAX(14, height / 4));
    lv_obj_align(shine, LV_ALIGN_TOP_LEFT, LV_MAX(10, width / 6), LV_MAX(8, height / 7));
    lv_obj_clear_flag(shine, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(shine, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(shine, 0, 0);
    lv_obj_set_style_bg_color(shine, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(shine, 88, 0);

    label = lv_label_create(root);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);

    return root;
}

lv_obj_t * gamepad_button_create_compact(lv_obj_t * parent, const char * text)
{
    return gamepad_button_create_compact_sized(parent, text, BUTTON_DEFAULT_W, BUTTON_DEFAULT_H);
}
