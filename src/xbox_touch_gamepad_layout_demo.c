#include "xbox_touch_gamepad_layout_demo.h"
#include "gamepad_mapping_selftest.h"
#include "gamepad_layout_model.h"
#include "gamepad_module_builders.h"
#include "gamepad_action_mapper.h"
#include "gamepad_input_state.h"
#include "gamepad_debug_overlay.h"
#include "gamepad_multitouch_input.h"

#if defined(ESP_PLATFORM)
#include "esp_log.h"
static const char * TAG = "gamepad_ui";
#else
#define ESP_LOGI(tag, fmt, ...) ((void)0)
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

/* LVGL 9.5 references used in this integration layer:
 * Coordinates: https://lvgl.io/docs/open/9.5/common-widget-features/coordinates
 * Events: https://lvgl.io/docs/open/9.5/common-widget-features/events
 * Slider: https://lvgl.io/docs/open/9.5/widgets/slider
 * Animation: https://lvgl.io/docs/open/9.5/main-modules/animation
 */

typedef struct {
    gamepad_module_instance_t * module;
    lv_obj_t * shell;
    lv_obj_t * content;
    lv_obj_t * tag;
    lv_obj_t * overlay;
    lv_obj_t * handle;
    lv_point_t press_point;
    lv_coord_t start_x;
    lv_coord_t start_y;
    lv_coord_t start_w;
    lv_coord_t start_h;
    bool dragging;
    bool resizing;
    bool changed;
    bool multitouch_active;
    bool multitouch_next_active;
} module_view_ctx_t;

typedef struct {
    uint8_t slot_id;
    bool save_action;
} slot_action_t;

typedef struct {
    lv_obj_t * screen;
    lv_obj_t * stage;
    lv_obj_t * drag_preview;
    lv_obj_t * toolbar;
    lv_obj_t * toolbar_buttons[6];
    lv_obj_t * library_panel;
    lv_obj_t * library_list;
    lv_obj_t * config_panel;
    lv_obj_t * slot_labels[3];
    lv_obj_t * status_label;
    lv_obj_t * active_msgbox;
    lv_obj_t * binding_panel;
    lv_obj_t * binding_dropdown;
    lv_obj_t * binding_current_label;
    lv_obj_t * touch_markers[GAMEPAD_MULTITOUCH_MAX_POINTS];
    lv_obj_t * touch_marker_labels[GAMEPAD_MULTITOUCH_MAX_POINTS];
    gamepad_action_id_t binding_options[GAMEPAD_ACTION_COUNT];
    uint8_t binding_option_count;
    bool binding_apply_guard;
    module_view_ctx_t * selected_ctx;
    gamepad_layout_config_t layout;
    uint8_t current_slot;
    uint8_t pending_load_slot;
    bool edit_mode;
    bool dirty;
    slot_action_t slot_actions[3][2];
    uint32_t pending_render_selected_id;
} layout_demo_ui_t;

enum {
    TOOLBAR_BTN_MODE = 0,
    TOOLBAR_BTN_LIBRARY,
    TOOLBAR_BTN_DELETE,
    TOOLBAR_BTN_BIND,
    TOOLBAR_BTN_SAVE,
    TOOLBAR_BTN_CONFIG
};

#define DECOR_BODY_RADIUS 64

static layout_demo_ui_t g_ui;

static void update_status_line(const char * detail);
static void render_modules(uint32_t selected_id);
static void refresh_slot_summaries(void);
static void refresh_toolbar_state(void);
static void set_selected_ctx(module_view_ctx_t * ctx);
static void toggle_library_panel(bool show);
static void toggle_config_panel(bool show);
static void toggle_binding_panel(bool show);
static void refresh_binding_panel(void);
static void load_slot_into_ui(uint8_t slot_id);
static void resize_handle_event_cb(lv_event_t * e);
static void module_overlay_event_cb(lv_event_t * e);
static void schedule_render_modules(uint32_t selected_id);
static void show_drag_preview(lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
static void hide_drag_preview(void);
static void set_subtree_clickable(lv_obj_t * obj, bool clickable);

static module_view_ctx_t * find_module_view(uint32_t module_id)
{
    uint32_t count = lv_obj_get_child_count(g_ui.stage);
    for(uint32_t i = 0; i < count; i++) {
        lv_obj_t * shell = lv_obj_get_child(g_ui.stage, i);
        module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(shell);
        if(ctx != NULL && ctx->module != NULL && ctx->module->id == module_id) return ctx;
    }
    return NULL;
}

static void multitouch_visual_frame_cb(const gamepad_multitouch_visual_point_t * points,
                                       uint8_t point_count,
                                       void * user_data)
{
    uint32_t child_count;
    (void)user_data;

    if(g_ui.stage == NULL) return;

    child_count = lv_obj_get_child_count(g_ui.stage);
    for(uint32_t i = 0; i < child_count; i++) {
        lv_obj_t * shell = lv_obj_get_child(g_ui.stage, i);
        module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(shell);
        if(ctx == NULL || ctx->content == NULL) continue;
        ctx->multitouch_next_active = false;
    }

    for(uint8_t i = 0; i < GAMEPAD_MULTITOUCH_MAX_POINTS; i++) {
        lv_obj_t * marker = g_ui.touch_markers[i];
        if(marker == NULL) continue;
        if(i >= point_count) {
            lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(marker, (lv_coord_t)points[i].x - 15, (lv_coord_t)points[i].y - 15);
        if(lv_obj_has_flag(marker, LV_OBJ_FLAG_HIDDEN)) {
            lv_label_set_text_fmt(g_ui.touch_marker_labels[i], "%u", (unsigned)points[i].track_id);
        }
        lv_obj_clear_flag(marker, LV_OBJ_FLAG_HIDDEN);

        if(points[i].module_id != 0U) {
            module_view_ctx_t * ctx = find_module_view(points[i].module_id);
            if(ctx != NULL && ctx->content != NULL) {
                ctx->multitouch_next_active = true;
                if(ctx->module->kind == GAMEPAD_MODULE_VIEW_SLIDER) {
                    slider_2d_set_multitouch_position(ctx->content, true,
                                                      (lv_coord_t)points[i].x,
                                                      (lv_coord_t)points[i].y);
                }
            }
        }
    }

    for(uint32_t i = 0; i < child_count; i++) {
        lv_obj_t * shell = lv_obj_get_child(g_ui.stage, i);
        module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(shell);
        if(ctx == NULL || ctx->content == NULL || ctx->multitouch_active == ctx->multitouch_next_active) continue;

        ctx->multitouch_active = ctx->multitouch_next_active;
        if(ctx->multitouch_active) {
            lv_obj_add_state(ctx->content, LV_STATE_PRESSED);
            lv_obj_set_style_outline_width(ctx->shell, 3, 0);
            lv_obj_set_style_outline_color(ctx->shell, lv_color_hex(0x67E8F9), 0);
            lv_obj_set_style_outline_pad(ctx->shell, 2, 0);
        }
        else {
            lv_obj_remove_state(ctx->content, LV_STATE_PRESSED);
            if(!g_ui.edit_mode) lv_obj_set_style_outline_width(ctx->shell, 0, 0);
            if(ctx->module->kind == GAMEPAD_MODULE_VIEW_SLIDER) {
                slider_2d_set_multitouch_position(ctx->content, false, 0, 0);
            }
        }
    }
}

static void create_multitouch_markers(void)
{
    static const uint32_t colors[GAMEPAD_MULTITOUCH_MAX_POINTS] = {
        0x22D3EE, 0xFACC15, 0xFB7185, 0x4ADE80, 0xC084FC
    };

    for(uint8_t i = 0; i < GAMEPAD_MULTITOUCH_MAX_POINTS; i++) {
        lv_obj_t * marker = lv_obj_create(g_ui.screen);
        lv_obj_remove_style_all(marker);
        lv_obj_set_size(marker, 30, 30);
        lv_obj_set_style_radius(marker, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(marker, LV_OPA_30, 0);
        lv_obj_set_style_bg_color(marker, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_border_width(marker, 3, 0);
        lv_obj_set_style_border_color(marker, lv_color_hex(colors[i]), 0);
        lv_obj_clear_flag(marker, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);

        g_ui.touch_markers[i] = marker;
        g_ui.touch_marker_labels[i] = lv_label_create(marker);
        lv_label_set_text_fmt(g_ui.touch_marker_labels[i], "%u", (unsigned)i);
        lv_obj_set_style_text_color(g_ui.touch_marker_labels[i], lv_color_white(), 0);
        lv_obj_center(g_ui.touch_marker_labels[i]);
    }
}

static void create_shell_background(lv_obj_t * parent)
{
    lv_obj_t * shell;
    lv_obj_t * left_grip;
    lv_obj_t * right_grip;
    lv_obj_t * center_band;

    shell = lv_obj_create(parent);
    lv_obj_set_size(shell, 960, 540);
    lv_obj_align(shell, LV_ALIGN_CENTER, 0, 18);
    lv_obj_clear_flag(shell, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(shell, DECOR_BODY_RADIUS, 0);
    lv_obj_set_style_border_width(shell, 1, 0);
    lv_obj_set_style_border_color(shell, lv_color_hex(0x24364E), 0);
    lv_obj_set_style_bg_color(shell, lv_color_hex(0x101A29), 0);
    lv_obj_set_style_bg_grad_color(shell, lv_color_hex(0x182538), 0);
    lv_obj_set_style_bg_grad_dir(shell, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(shell, 40, 0);
    lv_obj_set_style_shadow_color(shell, lv_color_hex(0x02060D), 0);
    lv_obj_set_style_pad_all(shell, 0, 0);

    left_grip = lv_obj_create(shell);
    lv_obj_set_size(left_grip, 250, 360);
    lv_obj_align(left_grip, LV_ALIGN_LEFT_MID, -56, 90);
    lv_obj_clear_flag(left_grip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(left_grip, 110, 0);
    lv_obj_set_style_border_width(left_grip, 0, 0);
    lv_obj_set_style_bg_color(left_grip, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(left_grip, LV_OPA_80, 0);
    lv_obj_set_style_shadow_width(left_grip, 0, 0);

    right_grip = lv_obj_create(shell);
    lv_obj_set_size(right_grip, 250, 360);
    lv_obj_align(right_grip, LV_ALIGN_RIGHT_MID, 56, 90);
    lv_obj_clear_flag(right_grip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(right_grip, 110, 0);
    lv_obj_set_style_border_width(right_grip, 0, 0);
    lv_obj_set_style_bg_color(right_grip, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(right_grip, LV_OPA_80, 0);
    lv_obj_set_style_shadow_width(right_grip, 0, 0);

    center_band = lv_obj_create(shell);
    lv_obj_set_size(center_band, 320, 88);
    lv_obj_align(center_band, LV_ALIGN_TOP_MID, 0, 104);
    lv_obj_clear_flag(center_band, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(center_band, 34, 0);
    lv_obj_set_style_border_width(center_band, 1, 0);
    lv_obj_set_style_border_color(center_band, lv_color_hex(0x2F4C70), 0);
    lv_obj_set_style_bg_color(center_band, lv_color_hex(0x0F1A28), 0);
    lv_obj_set_style_bg_opa(center_band, LV_OPA_70, 0);
}

static void close_active_msgbox(void)
{
    if(g_ui.active_msgbox != NULL) {
        lv_msgbox_close(g_ui.active_msgbox);
        g_ui.active_msgbox = NULL;
    }
}

static void msgbox_close_cb(lv_event_t * e)
{
    lv_obj_t * msgbox = (lv_obj_t *)lv_event_get_user_data(e);
    lv_msgbox_close(msgbox);
    if(g_ui.active_msgbox == msgbox) {
        g_ui.active_msgbox = NULL;
    }
}

static void show_info_msgbox(const char * title, const char * text)
{
    lv_obj_t * ok_button;

    close_active_msgbox();
    g_ui.active_msgbox = lv_msgbox_create(g_ui.screen);
    lv_msgbox_add_title(g_ui.active_msgbox, title);
    lv_msgbox_add_text(g_ui.active_msgbox, text);
    lv_msgbox_add_close_button(g_ui.active_msgbox);
    ok_button = lv_msgbox_add_footer_button(g_ui.active_msgbox, "OK");
    lv_obj_add_event_cb(ok_button, msgbox_close_cb, LV_EVENT_CLICKED, g_ui.active_msgbox);
    lv_obj_center(g_ui.active_msgbox);
}

static const gamepad_module_catalog_entry_t * catalog_entry_for_module(const gamepad_module_instance_t * module)
{
    return gamepad_layout_catalog_find(module->kind, module->preset_id);
}

static void update_status_line(const char * detail)
{
    char slot_text[24];

    if(g_ui.current_slot == 0U) {
        lv_snprintf(slot_text, sizeof(slot_text), "slot:none");
    }
    else {
        lv_snprintf(slot_text, sizeof(slot_text), "slot:%u", (unsigned)g_ui.current_slot);
    }

    lv_label_set_text_fmt(g_ui.status_label,
                          "%s  |  %s  |  modules:%u  |  %s%s",
                          g_ui.edit_mode ? "EDIT" : "PLAY",
                          slot_text,
                          (unsigned)g_ui.layout.module_count,
                          g_ui.dirty ? "dirty" : "saved",
                          detail ? detail : "");
}

static void set_selected_visual(module_view_ctx_t * ctx, bool selected)
{
    if(ctx == NULL) return;

    if(!g_ui.edit_mode) {
        lv_obj_set_style_border_width(ctx->shell, 0, 0);
        lv_obj_add_flag(ctx->overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctx->handle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctx->tag, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(ctx->overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->tag, LV_OBJ_FLAG_HIDDEN);

    if(selected) {
        lv_obj_set_style_border_width(ctx->shell, 2, 0);
        lv_obj_set_style_border_color(ctx->shell, lv_color_hex(0x38BDF8), 0);
        lv_obj_set_style_outline_width(ctx->shell, 2, 0);
        lv_obj_set_style_outline_color(ctx->shell, lv_color_hex(0x0EA5E9), 0);
        lv_obj_set_style_outline_pad(ctx->shell, 2, 0);
    }
    else {
        lv_obj_set_style_border_width(ctx->shell, 1, 0);
        lv_obj_set_style_border_color(ctx->shell, lv_color_hex(0x35506D), 0);
        lv_obj_set_style_outline_width(ctx->shell, 0, 0);
    }

    lv_obj_clear_flag(ctx->handle, LV_OBJ_FLAG_HIDDEN);
}

static void set_selected_ctx(module_view_ctx_t * ctx)
{
    if(g_ui.selected_ctx == ctx) {
        return;
    }

    if(g_ui.selected_ctx != NULL) {
        set_selected_visual(g_ui.selected_ctx, false);
    }
    g_ui.selected_ctx = ctx;
    if(g_ui.selected_ctx != NULL) {
        set_selected_visual(g_ui.selected_ctx, true);
    }
    refresh_binding_panel();
}

static void refresh_module_editability(void)
{
    uint32_t count = lv_obj_get_child_count(g_ui.stage);
    uint32_t i;

    for(i = 0; i < count; i++) {
        lv_obj_t * shell = lv_obj_get_child(g_ui.stage, i);
        module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(shell);
        set_selected_visual(ctx, ctx == g_ui.selected_ctx);
    }
}

static void show_drag_preview(lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_set_pos(g_ui.drag_preview, x, y);
    lv_obj_set_size(g_ui.drag_preview, w, h);
    lv_obj_clear_flag(g_ui.drag_preview, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_ui.drag_preview);
}

static void hide_drag_preview(void)
{
    lv_obj_add_flag(g_ui.drag_preview, LV_OBJ_FLAG_HIDDEN);
}

static void set_subtree_clickable(lv_obj_t * obj, bool clickable)
{
    uint32_t i;
    uint32_t child_count;

    if(obj == NULL) return;

    if(clickable) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }

    child_count = lv_obj_get_child_count(obj);
    for(i = 0; i < child_count; i++) {
        set_subtree_clickable(lv_obj_get_child(obj, i), clickable);
    }
}

static lv_color_t button_neutral_color(void)
{
    return lv_color_hex(0x0F172A);
}

static lv_obj_t * build_module_widget(lv_obj_t * parent, const gamepad_module_instance_t * module)
{
    const gamepad_module_catalog_entry_t * entry = catalog_entry_for_module(module);

    if(entry == NULL) return NULL;

    switch(module->kind) {
        case GAMEPAD_MODULE_BUTTON_SINGLE:
            return gamepad_button_create_compact_sized(parent, entry->output_label, module->w, module->h);
        case GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT:
            return shoulder_keys_create_left_compact_sized(parent, module->w, module->h);
        case GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT:
            return shoulder_keys_create_right_compact_sized(parent, module->w, module->h);
        case GAMEPAD_MODULE_JOYSTICK:
            return joystick_stick_create_compact_sized(parent, module->w, module->h);
        case GAMEPAD_MODULE_DPAD:
            return dpad_create_compact_sized(parent, module->w, module->h);
        case GAMEPAD_MODULE_VIEW_SLIDER:
            return slider_2d_create_compact_sized(parent, module->w, module->h);
        default:
            return NULL;
    }
}

static void module_view_delete_cb(lv_event_t * e)
{
    module_view_ctx_t * ctx = (module_view_ctx_t *)lv_event_get_user_data(e);
    if(g_ui.selected_ctx == ctx) {
        g_ui.selected_ctx = NULL;
    }
    lv_free(ctx);
}

static void async_render_modules_cb(void * user_data)
{
    LV_UNUSED(user_data);
    render_modules(g_ui.pending_render_selected_id);
}

static void schedule_render_modules(uint32_t selected_id)
{
    g_ui.pending_render_selected_id = selected_id;
    lv_async_call_cancel(async_render_modules_cb, NULL);
    lv_async_call(async_render_modules_cb, NULL);
}

static bool apply_candidate_rect(module_view_ctx_t * ctx, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    gamepad_layout_error_t err;
    if(!gamepad_layout_can_place(&g_ui.layout, ctx->module->id, x, y, w, h, &err)) {
        return false;
    }

    ctx->module->x = x;
    ctx->module->y = y;
    ctx->module->w = w;
    ctx->module->h = h;
    return true;
}

static void refresh_toolbar_state(void)
{
    lv_label_set_text(lv_obj_get_child(g_ui.toolbar_buttons[TOOLBAR_BTN_MODE], 0),
                      g_ui.edit_mode ? "Play Mode" : "Edit Mode");

    if(g_ui.edit_mode) {
        lv_obj_clear_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_LIBRARY], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_DELETE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_BIND], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_SAVE], LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_LIBRARY], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_DELETE], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_BIND], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.toolbar_buttons[TOOLBAR_BTN_SAVE], LV_OBJ_FLAG_HIDDEN);
        toggle_library_panel(false);
        toggle_binding_panel(false);
    }

    refresh_module_editability();
    refresh_slot_summaries();
}

static void render_modules(uint32_t selected_id)
{
    uint16_t i;
    uint16_t rendered_count = 0U;

    lv_obj_clean(g_ui.stage);
    g_ui.selected_ctx = NULL;

    /* 控件全部重建：清空映射表与残留输入态，随后逐个重新登记绑定。 */
    gamepad_action_mapper_reset();
    gamepad_multitouch_input_reset();
    gamepad_input_state_reset();

    for(i = 0; i < g_ui.layout.module_count; i++) {
        gamepad_module_instance_t * module = &g_ui.layout.modules[i];
        const gamepad_module_catalog_entry_t * entry = catalog_entry_for_module(module);
        module_view_ctx_t * ctx;
        lv_obj_t * shell;
        lv_obj_t * widget;
        lv_obj_t * tag;
        lv_obj_t * overlay;
        lv_obj_t * handle;

        if(!module->visible || entry == NULL) continue;

        ctx = (module_view_ctx_t *)lv_malloc_zeroed(sizeof(*ctx));
        if(ctx == NULL) continue;

        shell = lv_obj_create(g_ui.stage);
        ctx->module = module;
        ctx->shell = shell;
        lv_obj_set_user_data(shell, ctx);
        lv_obj_remove_style_all(shell);
        lv_obj_set_pos(shell, module->x, module->y);
        lv_obj_set_size(shell, module->w, module->h);
        lv_obj_set_style_bg_opa(shell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(shell, lv_color_hex(0x35506D), 0);
        lv_obj_set_style_border_width(shell, 1, 0);
        lv_obj_add_event_cb(shell, module_view_delete_cb, LV_EVENT_DELETE, ctx);

        widget = build_module_widget(shell, module);
        if(widget != NULL) {
            ctx->content = widget;
            lv_obj_set_pos(widget, 0, 0);
            /* 登记 控件根对象 -> 模块绑定，供控件事件回调经 mapper 写状态。 */
            gamepad_action_mapper_register(widget, module);
            gamepad_multitouch_input_register_module(module);
            if(g_ui.edit_mode) {
                set_subtree_clickable(widget, false);
            }
        }

        tag = lv_label_create(shell);
        ctx->tag = tag;
        lv_label_set_text(tag, entry->library_label);
        lv_obj_set_style_bg_color(tag, lv_color_hex(0x0B1220), 0);
        lv_obj_set_style_bg_opa(tag, LV_OPA_80, 0);
        lv_obj_set_style_text_color(tag, lv_color_hex(0xE2E8F0), 0);
        lv_obj_set_style_pad_left(tag, 6, 0);
        lv_obj_set_style_pad_right(tag, 6, 0);
        lv_obj_set_style_pad_top(tag, 3, 0);
        lv_obj_set_style_pad_bottom(tag, 3, 0);
        lv_obj_set_style_radius(tag, 10, 0);
        lv_obj_align(tag, LV_ALIGN_TOP_LEFT, 4, 4);

        overlay = lv_obj_create(shell);
        ctx->overlay = overlay;
        lv_obj_remove_style_all(overlay);
        lv_obj_set_size(overlay, module->w, module->h);
        lv_obj_set_pos(overlay, 0, 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(overlay, module_overlay_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_move_foreground(overlay);

        handle = lv_obj_create(shell);
        ctx->handle = handle;
        lv_obj_set_size(handle, 22, 22);
        lv_obj_align(handle, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
        lv_obj_clear_flag(handle, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(handle, 8, 0);
        lv_obj_set_style_bg_color(handle, lv_color_hex(0x38BDF8), 0);
        lv_obj_set_style_border_width(handle, 1, 0);
        lv_obj_set_style_border_color(handle, lv_color_hex(0xE0F2FE), 0);
        {
            lv_obj_t * handle_label = lv_label_create(handle);
            lv_label_set_text(handle_label, "+");
            lv_obj_set_style_text_color(handle_label, lv_color_white(), 0);
            lv_obj_center(handle_label);
        }
        lv_obj_add_event_cb(handle, resize_handle_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_move_foreground(handle);

        if(selected_id != 0U && module->id == selected_id) {
            g_ui.selected_ctx = ctx;
        }
        rendered_count++;
    }

    refresh_module_editability();
    ESP_LOGI(TAG, "render_modules complete: layout=%u rendered=%u selected=%lu",
             (unsigned)g_ui.layout.module_count,
             (unsigned)rendered_count,
             (unsigned long)selected_id);
}

static lv_coord_t clamp_coord(lv_coord_t value, lv_coord_t min, lv_coord_t max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static void module_finish_interaction(module_view_ctx_t * ctx)
{
    const gamepad_module_catalog_entry_t * entry = catalog_entry_for_module(ctx->module);
    lv_coord_t candidate_x = lv_obj_get_x(g_ui.drag_preview);
    lv_coord_t candidate_y = lv_obj_get_y(g_ui.drag_preview);
    lv_coord_t candidate_w = lv_obj_get_width(g_ui.drag_preview);
    lv_coord_t candidate_h = lv_obj_get_height(g_ui.drag_preview);
    bool size_changed = (candidate_w != ctx->module->w) || (candidate_h != ctx->module->h);

    hide_drag_preview();

    if(!ctx->changed) {
        update_status_line(" | ready");
        return;
    }

    if(entry == NULL) {
        return;
    }

    if(!apply_candidate_rect(ctx, candidate_x, candidate_y, candidate_w, candidate_h)) {
        update_status_line(" | reverted illegal move");
        return;
    }

    g_ui.dirty = true;
    if(size_changed) {
        schedule_render_modules(ctx->module->id);
    }
    else {
        lv_obj_set_pos(ctx->shell, ctx->module->x, ctx->module->y);
    }
    update_status_line(" | layout updated");
}

static void resize_handle_event_cb(lv_event_t * e)
{
    module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(lv_obj_get_parent(lv_event_get_target_obj(e)));
    const gamepad_module_catalog_entry_t * entry = catalog_entry_for_module(ctx->module);
    lv_event_code_t code = lv_event_get_code(e);

    if(!g_ui.edit_mode || ctx == NULL || entry == NULL) return;
    if(ctx->module->locked) return;   /* 锁定模块禁止缩放 */

    if(code == LV_EVENT_PRESSED) {
        lv_indev_t * indev = lv_event_get_indev(e);
        set_selected_ctx(ctx);
        ctx->resizing = true;
        ctx->changed = false;
        ctx->start_x = ctx->module->x;
        ctx->start_y = ctx->module->y;
        ctx->start_w = ctx->module->w;
        ctx->start_h = ctx->module->h;
        if(indev != NULL) {
            lv_indev_get_point(indev, &ctx->press_point);
        }
        show_drag_preview(ctx->start_x, ctx->start_y, ctx->start_w, ctx->start_h);
        return;
    }

    if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_point_t point;
        lv_coord_t min_w;
        lv_coord_t max_w;
        lv_coord_t new_w;
        lv_coord_t new_h;
        float aspect;
        float scale_from_width;

        if(indev == NULL) return;
        lv_indev_get_point(indev, &point);

        aspect = (float)entry->default_h / (float)entry->default_w;
        min_w = (entry->default_w * entry->min_scale_pct) / 100;
        max_w = (entry->default_w * entry->max_scale_pct) / 100;
        new_w = ctx->start_w + LV_MAX(point.x - ctx->press_point.x, point.y - ctx->press_point.y);
        new_w = clamp_coord(new_w, min_w, max_w);

        if(ctx->start_x + new_w > GAMEPAD_LAYOUT_SCREEN_W) {
            new_w = GAMEPAD_LAYOUT_SCREEN_W - ctx->start_x;
        }
        scale_from_width = (float)new_w / (float)entry->default_w;
        new_h = (lv_coord_t)(entry->default_h * scale_from_width + 0.5f);
        if(ctx->start_y + new_h > GAMEPAD_LAYOUT_SCREEN_H) {
            new_h = GAMEPAD_LAYOUT_SCREEN_H - ctx->start_y;
            new_w = (lv_coord_t)(new_h / aspect + 0.5f);
        }
        new_w = clamp_coord(new_w, min_w, max_w);
        scale_from_width = (float)new_w / (float)entry->default_w;
        new_h = (lv_coord_t)(entry->default_h * scale_from_width + 0.5f);

        show_drag_preview(ctx->start_x, ctx->start_y, new_w, new_h);
        ctx->changed = true;
        return;
    }

    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ctx->resizing = false;
        module_finish_interaction(ctx);
    }
}

static void module_overlay_event_cb(lv_event_t * e)
{
    lv_obj_t * overlay = lv_event_get_target_obj(e);
    lv_obj_t * shell = lv_obj_get_parent(overlay);
    module_view_ctx_t * ctx = (module_view_ctx_t *)lv_obj_get_user_data(shell);
    lv_event_code_t code = lv_event_get_code(e);

    if(!g_ui.edit_mode || ctx == NULL) return;

    if(code == LV_EVENT_PRESSED) {
        lv_indev_t * indev = lv_event_get_indev(e);
        set_selected_ctx(ctx);
        if(ctx->module->locked) {           /* 锁定模块可选中查看，但禁止拖动 */
            update_status_line(" | module locked");
            return;
        }
        ctx->dragging = true;
        ctx->changed = false;
        ctx->start_x = ctx->module->x;
        ctx->start_y = ctx->module->y;
        ctx->start_w = ctx->module->w;
        ctx->start_h = ctx->module->h;
        if(indev != NULL) {
            lv_indev_get_point(indev, &ctx->press_point);
        }
        show_drag_preview(ctx->start_x, ctx->start_y, ctx->start_w, ctx->start_h);
        return;
    }

    if(code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_event_get_indev(e);
        lv_point_t point;
        lv_coord_t new_x;
        lv_coord_t new_y;

        if(ctx->module->locked) return;     /* 锁定模块不响应拖动 */
        if(indev == NULL) return;
        lv_indev_get_point(indev, &point);
        new_x = clamp_coord(ctx->start_x + (point.x - ctx->press_point.x), 0, GAMEPAD_LAYOUT_SCREEN_W - ctx->start_w);
        new_y = clamp_coord(ctx->start_y + (point.y - ctx->press_point.y), 0, GAMEPAD_LAYOUT_SCREEN_H - ctx->start_h);
        show_drag_preview(new_x, new_y, ctx->start_w, ctx->start_h);
        ctx->changed = true;
        return;
    }

    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ctx->dragging = false;
        module_finish_interaction(ctx);
    }
}

static void refresh_slot_summaries(void)
{
    uint8_t slot_id;

    for(slot_id = 1U; slot_id <= 3U; slot_id++) {
        gamepad_layout_slot_summary_t summary;
        lv_obj_t * label = g_ui.slot_labels[slot_id - 1U];

        if(gamepad_layout_get_slot_summary(slot_id, &summary)) {
            lv_label_set_text_fmt(label,
                                  "Slot %u | %s | %u modules%s%s",
                                  (unsigned)slot_id,
                                  summary.name,
                                  (unsigned)summary.module_count,
                                  summary.is_active ? " | active" : "",
                                  summary.valid ? "" : " | invalid");
        }
        else {
            lv_label_set_text_fmt(label, "Slot %u | empty", (unsigned)slot_id);
        }
    }
}

static void toggle_library_panel(bool show)
{
    if(show) {
        lv_obj_clear_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN);
        if(g_ui.binding_panel != NULL) lv_obj_add_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_ui.library_panel);
    }
    else {
        lv_obj_add_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void toggle_config_panel(bool show)
{
    if(show) {
        lv_obj_clear_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN);
        if(g_ui.binding_panel != NULL) lv_obj_add_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_ui.config_panel);
    }
    else {
        lv_obj_add_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void toggle_binding_panel(bool show)
{
    if(g_ui.binding_panel == NULL) return;

    if(show) {
        lv_obj_clear_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_ui.binding_panel);
        refresh_binding_panel();
    }
    else {
        lv_obj_add_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static void save_to_slot(uint8_t slot_id)
{
    gamepad_layout_error_t err;

    if(!gamepad_layout_validate(&g_ui.layout, &err)) {
        show_info_msgbox("Cannot Save", err.message);
        return;
    }

    g_ui.layout.slot_id = slot_id;
    g_ui.layout.is_active = true;
    lv_snprintf(g_ui.layout.name, sizeof(g_ui.layout.name), "Layout %u", (unsigned)slot_id);

    if(!gamepad_layout_save_slot(slot_id, &g_ui.layout)) {
        show_info_msgbox("Save Failed", "Unable to save JSON layout file.");
        return;
    }

    g_ui.current_slot = slot_id;
    g_ui.dirty = false;
    refresh_slot_summaries();
    update_status_line(" | saved");
}

static void load_slot_into_ui(uint8_t slot_id)
{
    if(!gamepad_layout_load_slot(slot_id, &g_ui.layout)) {
        show_info_msgbox("Load Failed", "Selected slot is missing or invalid.");
        return;
    }

    g_ui.current_slot = slot_id;
    g_ui.dirty = false;
    schedule_render_modules(0U);
    refresh_slot_summaries();
    update_status_line(" | slot loaded");
}

static void pending_prompt_cancel_cb(lv_event_t * e)
{
    msgbox_close_cb(e);
}

static void pending_prompt_discard_cb(lv_event_t * e)
{
    msgbox_close_cb(e);
    load_slot_into_ui(g_ui.pending_load_slot);
}

static void pending_prompt_save_cb(lv_event_t * e)
{
    msgbox_close_cb(e);
    if(g_ui.current_slot == 0U) {
        save_to_slot(g_ui.pending_load_slot);
    }
    else {
        save_to_slot(g_ui.current_slot);
    }
    load_slot_into_ui(g_ui.pending_load_slot);
}

static void request_load_slot(uint8_t slot_id)
{
    lv_obj_t * save_button;
    lv_obj_t * discard_button;
    lv_obj_t * cancel_button;

    if(!g_ui.dirty) {
        load_slot_into_ui(slot_id);
        return;
    }

    g_ui.pending_load_slot = slot_id;
    close_active_msgbox();
    g_ui.active_msgbox = lv_msgbox_create(g_ui.screen);
    lv_msgbox_add_title(g_ui.active_msgbox, "Unsaved Layout");
    lv_msgbox_add_text(g_ui.active_msgbox, "Current layout has unsaved changes.");
    lv_msgbox_add_close_button(g_ui.active_msgbox);

    if(g_ui.current_slot != 0U) {
        save_button = lv_msgbox_add_footer_button(g_ui.active_msgbox, "Save & Load");
        lv_obj_add_event_cb(save_button, pending_prompt_save_cb, LV_EVENT_CLICKED, g_ui.active_msgbox);
    }
    discard_button = lv_msgbox_add_footer_button(g_ui.active_msgbox, "Discard");
    cancel_button = lv_msgbox_add_footer_button(g_ui.active_msgbox, "Cancel");
    lv_obj_add_event_cb(discard_button, pending_prompt_discard_cb, LV_EVENT_CLICKED, g_ui.active_msgbox);
    lv_obj_add_event_cb(cancel_button, pending_prompt_cancel_cb, LV_EVENT_CLICKED, g_ui.active_msgbox);
    lv_obj_center(g_ui.active_msgbox);
}

static void toolbar_button_event_cb(lv_event_t * e)
{
    uintptr_t button_id = (uintptr_t)lv_event_get_user_data(e);
    gamepad_layout_error_t err;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    switch(button_id) {
        case TOOLBAR_BTN_MODE:
            if(g_ui.edit_mode) {
                if(!gamepad_layout_validate(&g_ui.layout, &err)) {
                    show_info_msgbox("Invalid Layout", err.message);
                    return;
                }
                g_ui.edit_mode = false;
                gamepad_multitouch_input_set_enabled(true);
                set_selected_ctx(NULL);
                toggle_library_panel(false);
                toggle_config_panel(false);
                toggle_binding_panel(false);
            }
            else {
                g_ui.edit_mode = true;
                gamepad_multitouch_input_set_enabled(false);
            }
            refresh_toolbar_state();
            schedule_render_modules(0U);
            update_status_line(g_ui.edit_mode ? " | edit ready" : " | play ready");
            break;
        case TOOLBAR_BTN_LIBRARY:
            if(!g_ui.edit_mode) {
                show_info_msgbox("Edit Required", "Switch to edit mode before adding modules.");
                return;
            }
            toggle_library_panel(lv_obj_has_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN));
            break;
        case TOOLBAR_BTN_DELETE:
            if(!g_ui.edit_mode) {
                return;
            }
            if(g_ui.selected_ctx == NULL) {
                show_info_msgbox("Nothing Selected", "Select a module first.");
                return;
            }
            {
                uint32_t remove_id = g_ui.selected_ctx->module->id;
                set_selected_ctx(NULL);   /* 先清选中(内部刷新绑定面板)，再删除 */
                if(gamepad_layout_remove_module(&g_ui.layout, remove_id)) {
                    g_ui.dirty = true;
                    schedule_render_modules(0U);
                    update_status_line(" | module deleted");
                }
            }
            break;
        case TOOLBAR_BTN_BIND:
            if(!g_ui.edit_mode) {
                return;
            }
            toggle_binding_panel(lv_obj_has_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN));
            break;
        case TOOLBAR_BTN_SAVE:
            if(!g_ui.edit_mode) {
                return;
            }
            if(g_ui.current_slot == 0U) {
                toggle_config_panel(true);
                show_info_msgbox("Choose Slot", "Save the current layout to one of the three local slots.");
                return;
            }
            save_to_slot(g_ui.current_slot);
            break;
        case TOOLBAR_BTN_CONFIG:
            toggle_config_panel(lv_obj_has_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN));
            break;
        default:
            break;
    }
}

static void library_item_event_cb(lv_event_t * e)
{
    const gamepad_module_catalog_entry_t * entry = (const gamepad_module_catalog_entry_t *)lv_event_get_user_data(e);
    gamepad_layout_error_t err;

    if(lv_event_get_code(e) != LV_EVENT_CLICKED || entry == NULL) return;

    if(!gamepad_layout_try_add_module(&g_ui.layout, entry->kind, entry->preset_id, &err)) {
        show_info_msgbox("Cannot Add", err.message);
        return;
    }

    g_ui.dirty = true;
    schedule_render_modules(g_ui.layout.modules[g_ui.layout.module_count - 1U].id);
    update_status_line(" | module added");
}

static void slot_action_event_cb(lv_event_t * e)
{
    slot_action_t * action = (slot_action_t *)lv_event_get_user_data(e);

    if(lv_event_get_code(e) != LV_EVENT_CLICKED || action == NULL) return;

    if(action->save_action) {
        save_to_slot(action->slot_id);
    }
    else {
        request_load_slot(action->slot_id);
    }
}

static void restore_default_layout_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    /* Restore the built-in layout in memory; the user can save it explicitly afterwards. */
    gamepad_multitouch_input_set_enabled(false);
    close_active_msgbox();
    gamepad_layout_load_default(&g_ui.layout);
    g_ui.current_slot = 0U;
    g_ui.dirty = true;
    set_selected_ctx(NULL);
    schedule_render_modules(0U);
    refresh_slot_summaries();
    toggle_config_panel(false);
    if(!g_ui.edit_mode) {
        gamepad_multitouch_input_set_enabled(true);
    }
    update_status_line(" | default layout restored (unsaved)");
}

static lv_obj_t * create_toolbar_button(lv_obj_t * parent, const char * text, uintptr_t button_id)
{
    lv_obj_t * button = lv_button_create(parent);
    lv_obj_t * label = lv_label_create(button);

    lv_obj_set_height(button, 38);
    lv_obj_set_style_radius(button, 18, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x13253C), 0);
    lv_obj_set_style_bg_grad_color(button, lv_color_hex(0x1C3552), 0);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x34587B), 0);
    lv_obj_set_style_pad_left(button, 14, 0);
    lv_obj_set_style_pad_right(button, 14, 0);
    lv_obj_add_event_cb(button, toolbar_button_event_cb, LV_EVENT_CLICKED, (void *)button_id);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF8FAFC), 0);
    lv_obj_center(label);
    return button;
}

static void create_toolbar(void)
{
    lv_obj_t * title;

    g_ui.toolbar = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.toolbar, 640, 58);
    lv_obj_align(g_ui.toolbar, LV_ALIGN_TOP_MID, 0, 14);
    lv_obj_clear_flag(g_ui.toolbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_ui.toolbar, 28, 0);
    lv_obj_set_style_bg_color(g_ui.toolbar, lv_color_hex(0x0C1828), 0);
    lv_obj_set_style_bg_opa(g_ui.toolbar, LV_OPA_80, 0);
    lv_obj_set_style_border_width(g_ui.toolbar, 1, 0);
    lv_obj_set_style_border_color(g_ui.toolbar, lv_color_hex(0x2D4664), 0);
    lv_obj_set_style_shadow_width(g_ui.toolbar, 20, 0);
    lv_obj_set_style_shadow_color(g_ui.toolbar, lv_color_hex(0x02060D), 0);
    lv_obj_set_layout(g_ui.toolbar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_ui.toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_ui.toolbar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(g_ui.toolbar, 10, 0);

    title = lv_label_create(g_ui.toolbar);
    lv_label_set_text(title, "Xbox Touch Layout");
    lv_obj_set_style_text_color(title, lv_color_hex(0xD6E7FF), 0);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);

    g_ui.toolbar_buttons[TOOLBAR_BTN_MODE] = create_toolbar_button(g_ui.toolbar, "Play Mode", TOOLBAR_BTN_MODE);
    g_ui.toolbar_buttons[TOOLBAR_BTN_LIBRARY] = create_toolbar_button(g_ui.toolbar, "Library", TOOLBAR_BTN_LIBRARY);
    g_ui.toolbar_buttons[TOOLBAR_BTN_DELETE] = create_toolbar_button(g_ui.toolbar, "Delete", TOOLBAR_BTN_DELETE);
    g_ui.toolbar_buttons[TOOLBAR_BTN_BIND] = create_toolbar_button(g_ui.toolbar, "Bind", TOOLBAR_BTN_BIND);
    g_ui.toolbar_buttons[TOOLBAR_BTN_SAVE] = create_toolbar_button(g_ui.toolbar, "Save", TOOLBAR_BTN_SAVE);
    g_ui.toolbar_buttons[TOOLBAR_BTN_CONFIG] = create_toolbar_button(g_ui.toolbar, "Configs", TOOLBAR_BTN_CONFIG);
}

static void create_library_panel(void)
{
    const gamepad_module_catalog_entry_t * catalog;
    size_t count;
    size_t i;

    g_ui.library_panel = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.library_panel, 260, 440);
    lv_obj_align(g_ui.library_panel, LV_ALIGN_RIGHT_MID, -20, 12);
    lv_obj_clear_flag(g_ui.library_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_ui.library_panel, 24, 0);
    lv_obj_set_style_bg_color(g_ui.library_panel, lv_color_hex(0x0F1C2D), 0);
    lv_obj_set_style_bg_opa(g_ui.library_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(g_ui.library_panel, 1, 0);
    lv_obj_set_style_border_color(g_ui.library_panel, lv_color_hex(0x325472), 0);
    lv_obj_set_style_pad_all(g_ui.library_panel, 12, 0);
    lv_obj_add_flag(g_ui.library_panel, LV_OBJ_FLAG_HIDDEN);

    {
        lv_obj_t * title = lv_label_create(g_ui.library_panel);
        lv_label_set_text(title, "Official Module Library");
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    g_ui.library_list = lv_obj_create(g_ui.library_panel);
    lv_obj_set_size(g_ui.library_list, 236, 384);
    lv_obj_align(g_ui.library_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(g_ui.library_list, 18, 0);
    lv_obj_set_style_bg_color(g_ui.library_list, lv_color_hex(0x0B1624), 0);
    lv_obj_set_style_border_width(g_ui.library_list, 1, 0);
    lv_obj_set_style_border_color(g_ui.library_list, lv_color_hex(0x27425C), 0);
    lv_obj_set_layout(g_ui.library_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_ui.library_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_ui.library_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(g_ui.library_list, 10, 0);
    lv_obj_set_style_pad_gap(g_ui.library_list, 8, 0);

    catalog = gamepad_layout_catalog_get(&count);
    for(i = 0; i < count; i++) {
        lv_obj_t * button = lv_button_create(g_ui.library_list);
        lv_obj_t * label = lv_label_create(button);
        lv_obj_set_width(button, 208);
        lv_obj_set_height(button, 36);
        lv_obj_set_style_radius(button, 14, 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x13253C), 0);
        lv_obj_set_style_bg_grad_color(button, lv_color_hex(0x1C3552), 0);
        lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0x34587B), 0);
        lv_label_set_text(label, catalog[i].library_label);
        lv_obj_set_style_text_color(label, lv_color_hex(0xF8FAFC), 0);
        lv_obj_center(label);
        lv_obj_add_event_cb(button, library_item_event_cb, LV_EVENT_CLICKED, (void *)&catalog[i]);
    }
}

static void create_config_panel(void)
{
    uint8_t slot_id;

    g_ui.config_panel = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.config_panel, 320, 310);
    lv_obj_align(g_ui.config_panel, LV_ALIGN_LEFT_MID, 20, 18);
    lv_obj_clear_flag(g_ui.config_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_ui.config_panel, 24, 0);
    lv_obj_set_style_bg_color(g_ui.config_panel, lv_color_hex(0x0F1C2D), 0);
    lv_obj_set_style_bg_opa(g_ui.config_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(g_ui.config_panel, 1, 0);
    lv_obj_set_style_border_color(g_ui.config_panel, lv_color_hex(0x325472), 0);
    lv_obj_set_style_pad_all(g_ui.config_panel, 12, 0);
    lv_obj_add_flag(g_ui.config_panel, LV_OBJ_FLAG_HIDDEN);

    {
        lv_obj_t * title = lv_label_create(g_ui.config_panel);
        lv_label_set_text(title, "Local Layout Slots");
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    for(slot_id = 1U; slot_id <= 3U; slot_id++) {
        lv_obj_t * row = lv_obj_create(g_ui.config_panel);
        lv_obj_t * load_button;
        lv_obj_t * save_button;
        lv_obj_t * load_label;
        lv_obj_t * save_label;

        lv_obj_set_size(row, 292, 58);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 2, 34 + (slot_id - 1U) * 68);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row, 16, 0);
        lv_obj_set_style_bg_color(row, button_neutral_color(), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x22364D), 0);

        g_ui.slot_labels[slot_id - 1U] = lv_label_create(row);
        lv_obj_set_width(g_ui.slot_labels[slot_id - 1U], 160);
        lv_obj_set_style_text_color(g_ui.slot_labels[slot_id - 1U], lv_color_hex(0xCBD5E1), 0);
        lv_obj_align(g_ui.slot_labels[slot_id - 1U], LV_ALIGN_LEFT_MID, 10, 0);

        g_ui.slot_actions[slot_id - 1U][0].slot_id = slot_id;
        g_ui.slot_actions[slot_id - 1U][0].save_action = false;
        g_ui.slot_actions[slot_id - 1U][1].slot_id = slot_id;
        g_ui.slot_actions[slot_id - 1U][1].save_action = true;

        load_button = lv_button_create(row);
        lv_obj_set_size(load_button, 50, 32);
        lv_obj_align(load_button, LV_ALIGN_RIGHT_MID, -62, 0);
        lv_obj_add_event_cb(load_button, slot_action_event_cb, LV_EVENT_CLICKED, &g_ui.slot_actions[slot_id - 1U][0]);
        load_label = lv_label_create(load_button);
        lv_label_set_text(load_label, "Load");
        lv_obj_center(load_label);

        save_button = lv_button_create(row);
        lv_obj_set_size(save_button, 50, 32);
        lv_obj_align(save_button, LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_add_event_cb(save_button, slot_action_event_cb, LV_EVENT_CLICKED, &g_ui.slot_actions[slot_id - 1U][1]);
        save_label = lv_label_create(save_button);
        lv_label_set_text(save_label, "Save");
        lv_obj_center(save_label);
    }

    {
        lv_obj_t * restore_button = lv_button_create(g_ui.config_panel);
        lv_obj_t * restore_label = lv_label_create(restore_button);
        lv_obj_set_size(restore_button, 292, 38);
        lv_obj_align(restore_button, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_radius(restore_button, 16, 0);
        lv_obj_set_style_bg_color(restore_button, lv_color_hex(0x593535), 0);
        lv_obj_set_style_border_width(restore_button, 1, 0);
        lv_obj_set_style_border_color(restore_button, lv_color_hex(0xA85D5D), 0);
        lv_obj_add_event_cb(restore_button, restore_default_layout_event_cb, LV_EVENT_CLICKED, NULL);
        lv_label_set_text(restore_label, "Restore Default Layout");
        lv_obj_set_style_text_color(restore_label, lv_color_hex(0xFFE4E6), 0);
        lv_obj_center(restore_label);
    }
}

/* ===== Step 11: 编辑模式动作绑定 ===== */

/* 为选中模块的 kind 计算可选动作列表，写入 g_ui.binding_options，返回个数。 */
static uint8_t build_action_options_for_kind(gamepad_module_kind_t kind)
{
    uint8_t n = 0;

    if(kind == GAMEPAD_MODULE_JOYSTICK || kind == GAMEPAD_MODULE_VIEW_SLIDER) {
        g_ui.binding_options[n++] = GAMEPAD_ACTION_MOVE;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_LOOK;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_NONE;
    }
    else if(kind == GAMEPAD_MODULE_BUTTON_SINGLE) {
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_A;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_B;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_X;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_Y;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_L1;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_L2;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_R1;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_R2;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_START;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_SELECT;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_L3;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_R3;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_BTN_HOME;
        g_ui.binding_options[n++] = GAMEPAD_ACTION_NONE;
    }
    return n;
}

static void refresh_binding_panel(void)
{
    module_view_ctx_t * ctx = g_ui.selected_ctx;
    const gamepad_module_instance_t * m;
    const gamepad_module_catalog_entry_t * entry;
    char options[256];
    size_t pos = 0;
    uint8_t i;
    int sel_index = 0;

    if(g_ui.binding_panel == NULL) return;

    if(ctx == NULL || ctx->module == NULL) {
        g_ui.binding_option_count = 0;
        lv_label_set_text(g_ui.binding_current_label, "No module selected");
        lv_dropdown_set_options(g_ui.binding_dropdown, "-");
        lv_obj_add_state(g_ui.binding_dropdown, LV_STATE_DISABLED);
        return;
    }

    m = ctx->module;
    entry = catalog_entry_for_module(m);
    lv_label_set_text_fmt(g_ui.binding_current_label, "%s%s\nAction: %s",
                          entry ? entry->library_label : "Module",
                          m->locked ? " (locked)" : "",
                          gamepad_action_id_to_string(m->action_id));

    if(m->locked || !gamepad_action_is_remappable(m->kind)) {
        g_ui.binding_option_count = 0;
        lv_dropdown_set_options(g_ui.binding_dropdown, m->locked ? "(locked)" : "(fixed)");
        lv_obj_add_state(g_ui.binding_dropdown, LV_STATE_DISABLED);
        return;
    }

    g_ui.binding_option_count = build_action_options_for_kind(m->kind);
    options[0] = '\0';
    for(i = 0; i < g_ui.binding_option_count; i++) {
        int wrote = lv_snprintf(options + pos, sizeof(options) - pos, "%s%s",
                                (i > 0) ? "\n" : "",
                                gamepad_action_id_to_string(g_ui.binding_options[i]));
        if(wrote > 0) pos += (size_t)wrote;
        if(g_ui.binding_options[i] == m->action_id) sel_index = i;
    }
    lv_dropdown_set_options(g_ui.binding_dropdown, options);
    lv_obj_clear_state(g_ui.binding_dropdown, LV_STATE_DISABLED);

    g_ui.binding_apply_guard = true;
    lv_dropdown_set_selected(g_ui.binding_dropdown, (uint32_t)sel_index);
    g_ui.binding_apply_guard = false;
}

static void binding_dropdown_event_cb(lv_event_t * e)
{
    module_view_ctx_t * ctx;
    uint32_t sel;
    gamepad_action_id_t new_action;

    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if(g_ui.binding_apply_guard) return;

    ctx = g_ui.selected_ctx;
    if(ctx == NULL || ctx->module == NULL) return;
    if(ctx->module->locked) return;   /* 锁定模块禁止改绑定 */
    if(!gamepad_action_is_remappable(ctx->module->kind)) return;

    sel = lv_dropdown_get_selected(g_ui.binding_dropdown);
    if(sel >= g_ui.binding_option_count) return;

    new_action = g_ui.binding_options[sel];
    if(ctx->module->action_id == new_action) return;

    ctx->module->action_id = new_action;
    g_ui.dirty = true;
    /* 重建控件以让 mapper 用新绑定重新登记(注册表持有的是登记时的拷贝)。 */
    schedule_render_modules(ctx->module->id);
    refresh_binding_panel();
    update_status_line(" | binding updated");
}

static void create_binding_panel(void)
{
    lv_obj_t * title;

    g_ui.binding_panel = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.binding_panel, 300, 220);
    lv_obj_align(g_ui.binding_panel, LV_ALIGN_LEFT_MID, 20, 18);
    lv_obj_clear_flag(g_ui.binding_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_ui.binding_panel, 24, 0);
    lv_obj_set_style_bg_color(g_ui.binding_panel, lv_color_hex(0x0F1C2D), 0);
    lv_obj_set_style_bg_opa(g_ui.binding_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(g_ui.binding_panel, 1, 0);
    lv_obj_set_style_border_color(g_ui.binding_panel, lv_color_hex(0x325472), 0);
    lv_obj_set_style_pad_all(g_ui.binding_panel, 12, 0);
    lv_obj_add_flag(g_ui.binding_panel, LV_OBJ_FLAG_HIDDEN);

    title = lv_label_create(g_ui.binding_panel);
    lv_label_set_text(title, "Action Binding");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    g_ui.binding_current_label = lv_label_create(g_ui.binding_panel);
    lv_obj_set_width(g_ui.binding_current_label, 268);
    lv_obj_set_style_text_color(g_ui.binding_current_label, lv_color_hex(0xCBD5E1), 0);
    lv_obj_align(g_ui.binding_current_label, LV_ALIGN_TOP_LEFT, 0, 34);
    lv_label_set_text(g_ui.binding_current_label, "No module selected");

    g_ui.binding_dropdown = lv_dropdown_create(g_ui.binding_panel);
    lv_obj_set_width(g_ui.binding_dropdown, 268);
    lv_obj_align(g_ui.binding_dropdown, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_dropdown_set_options(g_ui.binding_dropdown, "-");
    lv_obj_add_event_cb(g_ui.binding_dropdown, binding_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void create_status_line(void)
{
    g_ui.status_label = lv_label_create(g_ui.screen);
    lv_obj_set_width(g_ui.status_label, 980);
    lv_obj_set_style_text_color(g_ui.status_label, lv_color_hex(0xC7D8EE), 0);
    lv_obj_set_style_text_align(g_ui.status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_ui.status_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    g_ui.drag_preview = lv_obj_create(g_ui.screen);
    lv_obj_remove_style_all(g_ui.drag_preview);
    lv_obj_set_style_bg_opa(g_ui.drag_preview, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(g_ui.drag_preview, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(g_ui.drag_preview, 2, 0);
    lv_obj_set_style_border_color(g_ui.drag_preview, lv_color_hex(0x7DD3FC), 0);
    lv_obj_set_style_outline_width(g_ui.drag_preview, 1, 0);
    lv_obj_set_style_outline_color(g_ui.drag_preview, lv_color_hex(0xE0F2FE), 0);
    lv_obj_add_flag(g_ui.drag_preview, LV_OBJ_FLAG_HIDDEN);
}

void xbox_touch_gamepad_layout_demo_create(void)
{
    bool loaded_from_slot;

    ESP_LOGI(TAG, "UI create begin");
    lv_memzero(&g_ui, sizeof(g_ui));
    g_ui.screen = lv_screen_active();
    g_ui.edit_mode = false;
    gamepad_multitouch_input_set_enabled(true);

    /* 先销毁可能残留的调试观察层(其定时器/标签指针需在清屏前失效)。 */
    gamepad_debug_overlay_destroy();

    lv_obj_clean(g_ui.screen);
    lv_obj_set_style_bg_color(g_ui.screen, lv_color_hex(0x07111D), 0);
    lv_obj_set_style_bg_grad_color(g_ui.screen, lv_color_hex(0x102238), 0);
    lv_obj_set_style_bg_grad_dir(g_ui.screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(g_ui.screen, LV_OPA_COVER, 0);

    create_shell_background(g_ui.screen);

    g_ui.stage = lv_obj_create(g_ui.screen);
    lv_obj_remove_style_all(g_ui.stage);
    lv_obj_set_size(g_ui.stage, GAMEPAD_LAYOUT_SCREEN_W, GAMEPAD_LAYOUT_SCREEN_H);
    lv_obj_set_pos(g_ui.stage, 0, 0);
    lv_obj_clear_flag(g_ui.stage, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    create_toolbar();
    create_library_panel();
    create_config_panel();
    create_binding_panel();
    create_status_line();
    gamepad_debug_overlay_create(g_ui.screen);
    create_multitouch_markers();
    gamepad_multitouch_input_set_visual_callback(multitouch_visual_frame_cb, NULL);

    loaded_from_slot = gamepad_layout_load_initial(&g_ui.layout, &g_ui.current_slot);
    ESP_LOGI(TAG, "layout loaded: source=%s slot=%u modules=%u",
             loaded_from_slot ? "slot" : "default",
             (unsigned)g_ui.current_slot,
             (unsigned)g_ui.layout.module_count);
    g_ui.dirty = false;
    if(!loaded_from_slot) {
        update_status_line(" | default layout");
    }

    render_modules(0U);
    refresh_slot_summaries();
    refresh_toolbar_state();
    update_status_line(loaded_from_slot ? " | startup layout ready" : " | default layout ready");
    ESP_LOGI(TAG, "UI create complete");
}

#if defined(XBOX_TOUCH_GAMEPAD_LAYOUT_DEMO_STANDALONE)

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
    sdl_hal_init(GAMEPAD_LAYOUT_SCREEN_W, GAMEPAD_LAYOUT_SCREEN_H);
    xbox_touch_gamepad_layout_demo_create();
#if defined(GAMEPAD_MAPPING_SELFTEST)
    gamepad_mapping_selftest_run_once();
#endif

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
