#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"

#include <stdbool.h>
#include <stdio.h>

#define DPAD_PANEL_SIZE          288
#define DPAD_KEY_SIZE             78
#define DPAD_HUB_SIZE             50
#define DPAD_KEY_INSET            18
#define DPAD_COMPACT_PANEL_SIZE  228
#define DPAD_COMPACT_KEY_SIZE     62
#define DPAD_COMPACT_HUB_SIZE     40
#define DPAD_COMPACT_KEY_INSET    14
#define DPAD_BASE_SCALE         LV_SCALE_NONE
#define DPAD_PRESS_SCALE          224

typedef struct {
    const char * name;
    const char * text;
    lv_color_t base_color;
    lv_color_t active_color;
    lv_obj_t * button;
    gamepad_dpad_dir_t dir;   /* 该键对应的方向 channel */
} dpad_key_t;

typedef struct {
    dpad_key_t keys[4];
} dpad_demo_t;

static void key_scale_anim_cb(void * var, int32_t value)
{
    dpad_key_t * key = (dpad_key_t *)var;
    lv_obj_set_style_transform_scale(key->button, value, 0);
}

static void key_color_anim_cb(void * var, int32_t value)
{
    dpad_key_t * key = (dpad_key_t *)var;
    lv_color_t mixed = lv_color_mix(key->active_color, key->base_color, (uint8_t)value);

    lv_obj_set_style_bg_color(key->button, mixed, 0);
    lv_obj_set_style_shadow_color(key->button, mixed, 0);
}

static void start_key_scale_anim(dpad_key_t * key, int32_t start, int32_t end, uint32_t duration)
{
    lv_anim_t anim;

    lv_anim_delete(key, key_scale_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, key_scale_anim_cb);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void start_key_color_anim(dpad_key_t * key, uint32_t duration, bool pressed)
{
    lv_anim_t anim;

    lv_anim_delete(key, key_color_anim_cb);
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, key);
    lv_anim_set_exec_cb(&anim, key_color_anim_cb);
    lv_anim_set_values(&anim, pressed ? 0 : 255, pressed ? 255 : 0);
    lv_anim_set_duration(&anim, duration);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

static void dpad_key_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    dpad_key_t * key = (dpad_key_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        start_key_scale_anim(key, DPAD_BASE_SCALE, DPAD_PRESS_SCALE, 90);
        start_key_color_anim(key, 120, true);
        {
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(key->button);
            if(binding != NULL) gamepad_action_mapper_report_button(binding, (uint8_t)key->dir, true);
        }
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        start_key_scale_anim(key, DPAD_PRESS_SCALE, DPAD_BASE_SCALE, 140);
        start_key_color_anim(key, 160, false);
        {
            /* PRESS_LOST(滑出按钮)也必须清零，避免方向卡住 */
            const gamepad_action_binding_t * binding = gamepad_action_mapper_find(key->button);
            if(binding != NULL) gamepad_action_mapper_report_button(binding, (uint8_t)key->dir, false);
        }
    }
    else if(code == LV_EVENT_CLICKED) {
#if defined(GAMEPAD_INPUT_DEBUG_LOG)
        printf("%s pressed\n", key->name);
        fflush(stdout);
#endif
    }
}

static void dpad_setup_defaults(dpad_demo_t * demo)
{
    demo->keys[0].name = "UP";
    demo->keys[0].text = "UP";
    demo->keys[0].dir = GAMEPAD_DPAD_UP;
    demo->keys[0].base_color = lv_color_hex(0x3B82F6);
    demo->keys[0].active_color = lv_color_hex(0x60A5FA);

    demo->keys[1].name = "DOWN";
    demo->keys[1].text = "DOWN";
    demo->keys[1].dir = GAMEPAD_DPAD_DOWN;
    demo->keys[1].base_color = lv_color_hex(0x10B981);
    demo->keys[1].active_color = lv_color_hex(0x34D399);

    demo->keys[2].name = "LEFT";
    demo->keys[2].text = "LEFT";
    demo->keys[2].dir = GAMEPAD_DPAD_LEFT;
    demo->keys[2].base_color = lv_color_hex(0xF59E0B);
    demo->keys[2].active_color = lv_color_hex(0xFBBF24);

    demo->keys[3].name = "RIGHT";
    demo->keys[3].text = "RIGHT";
    demo->keys[3].dir = GAMEPAD_DPAD_RIGHT;
    demo->keys[3].base_color = lv_color_hex(0xEF4444);
    demo->keys[3].active_color = lv_color_hex(0xF87171);
}

static void create_dpad_key(lv_obj_t * parent, dpad_key_t * key, int32_t x, int32_t y, int32_t key_size)
{
    lv_obj_t * label;

    key->button = lv_button_create(parent);
    lv_obj_set_size(key->button, key_size, key_size);
    lv_obj_set_pos(key->button, x, y);
    lv_obj_set_style_radius(key->button, LV_MAX(16, key_size / 4), 0);
    lv_obj_set_style_bg_color(key->button, key->base_color, 0);
    lv_obj_set_style_border_width(key->button, 2, 0);
    lv_obj_set_style_border_color(key->button, lv_color_white(), 0);
    lv_obj_set_style_shadow_width(key->button, LV_MAX(12, key_size / 4), 0);
    lv_obj_set_style_shadow_opa(key->button, LV_OPA_30, 0);
    lv_obj_set_style_shadow_color(key->button, key->base_color, 0);
    lv_obj_set_style_transform_pivot_x(key->button, key_size / 2, 0);
    lv_obj_set_style_transform_pivot_y(key->button, key_size / 2, 0);
    lv_obj_set_style_transform_scale(key->button, DPAD_BASE_SCALE, 0);

    lv_obj_add_event_cb(key->button, dpad_key_event_cb, LV_EVENT_PRESSED, key);
    lv_obj_add_event_cb(key->button, dpad_key_event_cb, LV_EVENT_RELEASED, key);
    lv_obj_add_event_cb(key->button, dpad_key_event_cb, LV_EVENT_PRESS_LOST, key);
    lv_obj_add_event_cb(key->button, dpad_key_event_cb, LV_EVENT_CLICKED, key);

    label = lv_label_create(key->button);
    lv_label_set_text(label, key->text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);
}

static void dpad_panel_delete_cb(lv_event_t * e)
{
    dpad_demo_t * demo = (dpad_demo_t *)lv_event_get_user_data(e);
    uint32_t i;
    for(i = 0; i < 4U; i++) {
        lv_anim_delete(&demo->keys[i], key_scale_anim_cb);
        lv_anim_delete(&demo->keys[i], key_color_anim_cb);
    }
    lv_free(demo);
}

static lv_obj_t * dpad_build(lv_obj_t * parent, lv_coord_t panel_width, lv_coord_t panel_height)
{
    dpad_demo_t * demo = (dpad_demo_t *)lv_malloc_zeroed(sizeof(*demo));
    lv_obj_t * panel;
    lv_obj_t * hub;
    lv_obj_t * hub_label;
    int32_t key_mid;
    int32_t key_end;
    int32_t panel_size;
    int32_t key_size;
    int32_t hub_size;
    int32_t key_inset;
    int32_t key_offset_x;
    float scale;

    if(demo == NULL) return NULL;
    dpad_setup_defaults(demo);

    panel_size = LV_MIN(panel_width, panel_height);
    scale = (float)panel_size / (float)DPAD_COMPACT_PANEL_SIZE;
    key_size = LV_MAX(40, (int32_t)(DPAD_COMPACT_KEY_SIZE * scale));
    hub_size = LV_MAX(28, (int32_t)(DPAD_COMPACT_HUB_SIZE * scale));
    key_inset = LV_MAX(10, (int32_t)(DPAD_COMPACT_KEY_INSET * scale));
    key_offset_x = (int32_t)(-10.0f * scale);

    panel = lv_obj_create(parent);
    lv_obj_set_size(panel, panel_size, panel_size);
    lv_obj_set_style_radius(panel, LV_MAX(24, panel_size / 6), 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1F2937), 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_shadow_width(panel, LV_MAX(18, panel_size / 8), 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(panel, lv_color_black(), 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(panel, dpad_panel_delete_cb, LV_EVENT_DELETE, demo);

    key_mid = (panel_size - key_size) / 2;
    key_end = panel_size - key_size - key_inset;

    create_dpad_key(panel, &demo->keys[0], key_mid + key_offset_x, key_inset, key_size);
    create_dpad_key(panel, &demo->keys[1], key_mid + key_offset_x, key_end, key_size);
    create_dpad_key(panel, &demo->keys[2], key_inset + key_offset_x, key_mid, key_size);
    create_dpad_key(panel, &demo->keys[3], key_end + key_offset_x, key_mid, key_size);

    hub = lv_obj_create(panel);
    lv_obj_set_size(hub, hub_size, hub_size);
    lv_obj_center(hub);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_width(hub, 2, 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(0x334155), 0);
    lv_obj_set_style_shadow_width(hub, LV_MAX(8, hub_size / 3), 0);
    lv_obj_set_style_shadow_opa(hub, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(hub, lv_color_black(), 0);
    lv_obj_clear_flag(hub, LV_OBJ_FLAG_SCROLLABLE);

    hub_label = lv_label_create(hub);
    lv_label_set_text(hub_label, "OK");
    lv_obj_set_style_text_color(hub_label, lv_color_hex(0xE2E8F0), 0);
    lv_obj_center(hub_label);

    return panel;
}

lv_obj_t * dpad_create_compact(lv_obj_t * parent)
{
    return dpad_build(parent, DPAD_COMPACT_PANEL_SIZE, DPAD_COMPACT_PANEL_SIZE);
}

lv_obj_t * dpad_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    return dpad_build(parent, width, height);
}

void joystick_demo_create(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_t * panel;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101522), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x192235), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);

    panel = dpad_build(scr, DPAD_PANEL_SIZE, DPAD_PANEL_SIZE);
    lv_obj_center(panel);
    lv_obj_set_x(panel, lv_obj_get_x(panel) + 10);
}
