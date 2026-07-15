#include "gamepad_multitouch_input.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "gamepad_action_mapper.h"
#include "gamepad_input_state.h"
#include "gamepad_view_motion.h"
#if defined(ESP_PLATFORM)
#include "esp_timer.h"
#endif

#if defined(ESP_PLATFORM)
#include "esp_log.h"
static const char * TAG = "gamepad_multitouch";
#else
#define ESP_LOGI(tag, fmt, ...) ((void)0)
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

#define MULTITOUCH_MAX_MODULES GAMEPAD_LAYOUT_MAX_MODULES
#define MULTITOUCH_MAX_POINTS  GAMEPAD_MULTITOUCH_MAX_POINTS

typedef struct {
    bool used;
    gamepad_module_instance_t module;
    gamepad_action_binding_t binding;
    gamepad_view_motion_state_t view_motion;
    uint8_t view_track_id;
    uint32_t view_last_log_ms;
} multitouch_module_t;

typedef struct {
    bool active;
    float move_x;
    float move_y;
    float look_x;
    float look_y;
    bool buttons[GAMEPAD_BTN_COUNT];
    bool dpad[GAMEPAD_DPAD_COUNT];
} multitouch_frame_state_t;

static multitouch_module_t s_modules[MULTITOUCH_MAX_MODULES];
static bool s_enabled = false;
static gamepad_multitouch_visual_cb_t s_visual_callback = NULL;
static void * s_visual_user_data = NULL;
static uint8_t s_previous_track_ids[MULTITOUCH_MAX_POINTS];
static uint8_t s_previous_track_count;
static const gamepad_view_motion_config_t s_view_motion_config = {
    .pixels_per_ms_for_full_scale = 0.80f,
    .deadzone_velocity = 0.010f,
    .exponent = 1.0f,
    .max_output = 1.0f,
    .filter_tau_seconds = 0.010f,
};

static void clear_view_states(void)
{
    for(size_t i = 0; i < MULTITOUCH_MAX_MODULES; i++) {
        gamepad_view_motion_reset(&s_modules[i].view_motion);
    }
    s_previous_track_count = 0;
}

static void fill_binding(gamepad_action_binding_t * binding, const gamepad_module_instance_t * module)
{
    if(binding == NULL || module == NULL) return;
    binding->action_id = module->action_id;
    binding->kind = module->kind;
    binding->preset_id = module->preset_id;
    binding->deadzone = module->deadzone;
    binding->sensitivity = module->sensitivity;
}

void gamepad_multitouch_input_reset(void)
{
    memset(s_modules, 0, sizeof(s_modules));
    clear_view_states();
    if(s_visual_callback != NULL) s_visual_callback(NULL, 0, s_visual_user_data);
}

void gamepad_multitouch_input_register_module(const gamepad_module_instance_t * module)
{
    size_t i;

    if(module == NULL || !module->visible) return;

    for(i = 0; i < MULTITOUCH_MAX_MODULES; i++) {
        if(s_modules[i].used) continue;
        s_modules[i].used = true;
        s_modules[i].module = *module;
        fill_binding(&s_modules[i].binding, module);
        return;
    }

    ESP_LOGW(TAG, "module registry full, module id=%lu ignored", (unsigned long)module->id);
}

void gamepad_multitouch_input_set_enabled(bool enabled)
{
    if(s_enabled == enabled) return;
    s_enabled = enabled;
    if(!s_enabled) {
        clear_view_states();
        gamepad_input_state_reset();
        if(s_visual_callback != NULL) s_visual_callback(NULL, 0, s_visual_user_data);
    }
    ESP_LOGI(TAG, "multi-touch gamepad input %s", enabled ? "enabled" : "disabled");
}

bool gamepad_multitouch_input_is_enabled(void)
{
    return s_enabled;
}

void gamepad_multitouch_input_set_visual_callback(gamepad_multitouch_visual_cb_t callback,
                                                  void * user_data)
{
    s_visual_callback = callback;
    s_visual_user_data = user_data;
}

static bool point_in_module(const gamepad_module_instance_t * module, int32_t x, int32_t y)
{
    if(module == NULL || !module->visible) return false;
    return x >= module->x && y >= module->y &&
           x < (module->x + module->w) && y < (module->y + module->h);
}

static float clampf_local(float value, float lo, float hi)
{
    if(value < lo) return lo;
    if(value > hi) return hi;
    return value;
}

static void accumulate_button(multitouch_frame_state_t * frame,
                              const gamepad_action_binding_t * binding,
                              uint8_t channel)
{
    gamepad_button_id_t button;

    if(frame == NULL || binding == NULL) return;

    if(binding->action_id == GAMEPAD_ACTION_DPAD) {
        if(channel < GAMEPAD_DPAD_COUNT) {
            frame->dpad[channel] = true;
        }
        return;
    }

    switch(binding->action_id) {
        case GAMEPAD_ACTION_BTN_A:      button = GAMEPAD_BTN_A; break;
        case GAMEPAD_ACTION_BTN_B:      button = GAMEPAD_BTN_B; break;
        case GAMEPAD_ACTION_BTN_X:      button = GAMEPAD_BTN_X; break;
        case GAMEPAD_ACTION_BTN_Y:      button = GAMEPAD_BTN_Y; break;
        case GAMEPAD_ACTION_BTN_START:  button = GAMEPAD_BTN_START; break;
        case GAMEPAD_ACTION_BTN_SELECT: button = GAMEPAD_BTN_SELECT; break;
        case GAMEPAD_ACTION_BTN_L3:     button = GAMEPAD_BTN_L3; break;
        case GAMEPAD_ACTION_BTN_R3:     button = GAMEPAD_BTN_R3; break;
        case GAMEPAD_ACTION_BTN_HOME:   button = GAMEPAD_BTN_HOME; break;
        case GAMEPAD_ACTION_BTN_L1:
            button = (channel == GAMEPAD_CHANNEL_TRIGGER) ? GAMEPAD_BTN_L2 : GAMEPAD_BTN_L1;
            break;
        case GAMEPAD_ACTION_BTN_L2:
            button = GAMEPAD_BTN_L2;
            break;
        case GAMEPAD_ACTION_BTN_R1:
            button = (channel == GAMEPAD_CHANNEL_TRIGGER) ? GAMEPAD_BTN_R2 : GAMEPAD_BTN_R1;
            break;
        case GAMEPAD_ACTION_BTN_R2:
            button = GAMEPAD_BTN_R2;
            break;
        default:
            return;
    }

    if(button < GAMEPAD_BTN_COUNT) {
        frame->buttons[button] = true;
    }
}

static void accumulate_analog(multitouch_frame_state_t * frame,
                              const gamepad_action_binding_t * binding,
                              float nx,
                              float ny)
{
    float ox = 0.0f;
    float oy = 0.0f;
    int16_t deadzone;
    int16_t sensitivity;
    float dz;
    float sens;
    float mag;

    if(frame == NULL || binding == NULL) return;

    deadzone = binding->deadzone;
    sensitivity = binding->sensitivity;
    if(deadzone < 0) deadzone = 0;
    if(deadzone > GAMEPAD_DEADZONE_MAX) deadzone = GAMEPAD_DEADZONE_MAX;
    if(sensitivity < GAMEPAD_SENSITIVITY_MIN) sensitivity = GAMEPAD_SENSITIVITY_MIN;
    if(sensitivity > GAMEPAD_SENSITIVITY_MAX) sensitivity = GAMEPAD_SENSITIVITY_MAX;

    dz = (float)deadzone / 100.0f;
    sens = (float)sensitivity / 100.0f;
    if(dz > 0.95f) dz = 0.95f;

    nx = clampf_local(nx, -1.0f, 1.0f);
    ny = clampf_local(ny, -1.0f, 1.0f);
    mag = sqrtf(nx * nx + ny * ny);

    if(mag > dz && mag > 1e-6f) {
        float scaled = (mag - dz) / (1.0f - dz);
        float factor = (scaled / mag) * sens;
        ox = clampf_local(nx * factor, -1.0f, 1.0f);
        oy = clampf_local(ny * factor, -1.0f, 1.0f);
    }

    switch(binding->action_id) {
        case GAMEPAD_ACTION_MOVE:
            frame->move_x = ox;
            frame->move_y = oy;
            break;
        case GAMEPAD_ACTION_LOOK:
            frame->look_x = ox;
            frame->look_y = oy;
            break;
        default:
            break;
    }
}

static void handle_button_single(multitouch_frame_state_t * frame,
                                 const multitouch_module_t * entry)
{
    accumulate_button(frame, &entry->binding, GAMEPAD_CHANNEL_PRIMARY);
}

static void handle_dpad(multitouch_frame_state_t * frame,
                        const multitouch_module_t * entry,
                        int32_t x,
                        int32_t y)
{
    const gamepad_module_instance_t * m = &entry->module;
    int32_t local_x = x - m->x;
    int32_t local_y = y - m->y;
    int32_t center_x = m->w / 2;
    int32_t center_y = m->h / 2;
    int32_t dx = local_x - center_x;
    int32_t dy = local_y - center_y;
    int32_t abs_dx = dx >= 0 ? dx : -dx;
    int32_t abs_dy = dy >= 0 ? dy : -dy;
    int32_t dead = (m->w < m->h ? m->w : m->h) / 8;

    if(abs_dx < dead && abs_dy < dead) return;

    if(abs_dy >= abs_dx) {
        accumulate_button(frame, &entry->binding, dy < 0 ? GAMEPAD_DPAD_UP : GAMEPAD_DPAD_DOWN);
    }
    if(abs_dx >= abs_dy) {
        accumulate_button(frame, &entry->binding, dx < 0 ? GAMEPAD_DPAD_LEFT : GAMEPAD_DPAD_RIGHT);
    }
}

static void handle_shoulder(multitouch_frame_state_t * frame,
                            const multitouch_module_t * entry,
                            int32_t y)
{
    const gamepad_module_instance_t * m = &entry->module;
    int32_t local_y = y - m->y;
    uint8_t channel = (local_y >= (m->h / 2)) ? GAMEPAD_CHANNEL_TRIGGER : GAMEPAD_CHANNEL_PRIMARY;
    accumulate_button(frame, &entry->binding, channel);
}

static void handle_joystick(multitouch_frame_state_t * frame,
                            const multitouch_module_t * entry,
                            int32_t x,
                            int32_t y)
{
    const gamepad_module_instance_t * m = &entry->module;
    float half_w = (float)m->w / 2.0f;
    float half_h = (float)m->h / 2.0f;
    float radius = (half_w < half_h ? half_w : half_h) * 0.72f;
    float nx;
    float ny;

    if(radius <= 1.0f) return;

    nx = ((float)x - ((float)m->x + half_w)) / radius;
    ny = (((float)m->y + half_h) - (float)y) / radius;
    accumulate_analog(frame, &entry->binding, nx, ny);
}

static uint32_t find_top_module(int32_t x, int32_t y)
{
    size_t i;

    /* Iterate in reverse registration order: visually later modules are on top. */
    for(i = MULTITOUCH_MAX_MODULES; i > 0; i--) {
        const multitouch_module_t * entry = &s_modules[i - 1U];
        if(!entry->used) continue;
        if(!point_in_module(&entry->module, x, y)) continue;
        return entry->module.id;
    }
    return 0U;
}

static multitouch_module_t * find_module(uint32_t id)
{
    for(size_t i = 0; i < MULTITOUCH_MAX_MODULES; i++) {
        if(s_modules[i].used && s_modules[i].module.id == id) return &s_modules[i];
    }
    return NULL;
}

static bool track_was_present(uint8_t track_id)
{
    for(uint8_t i = 0; i < s_previous_track_count; i++) {
        if(s_previous_track_ids[i] == track_id) return true;
    }
    return false;
}

static uint32_t accumulate_point(multitouch_frame_state_t * frame, int32_t x, int32_t y)
{
    size_t i;
    if(frame == NULL) return 0U;
    for(i = MULTITOUCH_MAX_MODULES; i > 0; i--) {
        const multitouch_module_t * entry = &s_modules[i - 1U];
        if(!entry->used) continue;
        if(!point_in_module(&entry->module, x, y)) continue;
        if(entry->module.kind == GAMEPAD_MODULE_VIEW_SLIDER) continue;

        switch(entry->module.kind) {
            case GAMEPAD_MODULE_BUTTON_SINGLE:
                handle_button_single(frame, entry);
                break;
            case GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT:
            case GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT:
                handle_shoulder(frame, entry, y);
                break;
            case GAMEPAD_MODULE_JOYSTICK:
                handle_joystick(frame, entry, x, y);
                break;
            case GAMEPAD_MODULE_DPAD:
                handle_dpad(frame, entry, x, y);
                break;
            default:
                break;
        }
        return entry->module.id;
    }
    return 0U;
}

static void publish_frame(const multitouch_frame_state_t * frame)
{
    gamepad_input_set_move(frame->move_x, frame->move_y);
    gamepad_input_set_look(frame->look_x, frame->look_y);

    for(gamepad_button_id_t b = 0; b < GAMEPAD_BTN_COUNT; b++) {
        gamepad_input_set_button(b, frame->buttons[b]);
    }
    for(gamepad_dpad_dir_t d = 0; d < GAMEPAD_DPAD_COUNT; d++) {
        gamepad_input_set_dpad(d, frame->dpad[d]);
    }
}

void gamepad_multitouch_input_on_points(const uint16_t * x,
                                        const uint16_t * y,
                                        const uint8_t * track_id,
                                        uint8_t point_count)
{
    uint32_t now_ms;
#if defined(ESP_PLATFORM)
    now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
#else
    static uint32_t host_time_ms;
    host_time_ms += 8U;
    now_ms = host_time_ms;
#endif
    gamepad_multitouch_input_on_points_at_time(x, y, track_id, point_count, now_ms);
}

void gamepad_multitouch_input_on_points_at_time(const uint16_t * x,
                                                const uint16_t * y,
                                                const uint8_t * track_id,
                                                uint8_t point_count,
                                                uint32_t now_ms)
{
    multitouch_frame_state_t frame;
    gamepad_multitouch_visual_point_t visual_points[MULTITOUCH_MAX_POINTS];
    uint8_t accepted_count = 0;
    bool claimed[MULTITOUCH_MAX_POINTS] = {false};
    bool released[MULTITOUCH_MAX_MODULES] = {false};
    uint32_t claimed_module_ids[MULTITOUCH_MAX_POINTS] = {0U};
    uint8_t i;

    if(!s_enabled) return;

    memset(&frame, 0, sizeof(frame));
    frame.active = point_count > 0;

    for(uint8_t m = 0; m < MULTITOUCH_MAX_MODULES; m++) {
            multitouch_module_t * entry = &s_modules[m];
            bool found = false;
            if(!entry->used || entry->module.kind != GAMEPAD_MODULE_VIEW_SLIDER || !entry->view_motion.active) continue;
            for(i = 0; x != NULL && y != NULL && i < point_count && i < MULTITOUCH_MAX_POINTS; i++) {
                uint8_t id = track_id != NULL ? track_id[i] : i;
                if(id != entry->view_track_id) continue;
                found = true;
                claimed[i] = true;
                claimed_module_ids[i] = entry->module.id;
                const int32_t dx = (int32_t)x[i] - entry->view_motion.last_x;
                const int32_t dy = entry->view_motion.last_y - (int32_t)y[i];
                const uint32_t dt_ms = now_ms - entry->view_motion.last_time_ms;
                gamepad_view_motion_output_t look = gamepad_view_motion_update(&entry->view_motion,
                                                                                &s_view_motion_config,
                                                                                x[i],
                                                                                y[i],
                                                                                now_ms);
                frame.look_x = look.x;
                frame.look_y = look.y;
                if((dx != 0 || dy != 0) && (now_ms - entry->view_last_log_ms) >= 100U) {
                    const gamepad_view_motion_output_t target =
                        gamepad_view_motion_calculate_target(&s_view_motion_config, dx, dy, dt_ms);
                    (void)target;
                    ESP_LOGI(TAG,
                             "VIEW id=%u dx=%ld dy=%ld dt=%lu target=(%.3f,%.3f) filtered=(%.3f,%.3f)",
                             (unsigned)entry->view_track_id,
                             (long)dx,
                             (long)dy,
                             (unsigned long)dt_ms,
                             (double)target.x,
                             (double)target.y,
                             (double)look.x,
                             (double)look.y);
                    entry->view_last_log_ms = now_ms;
                }
                break;
            }
            if(!found) {
                ESP_LOGI(TAG, "VIEW release module=%lu id=%u",
                         (unsigned long)entry->module.id,
                         (unsigned)entry->view_track_id);
                (void)gamepad_view_motion_release(&entry->view_motion);
                released[m] = true;
            }
    }
    if(x != NULL && y != NULL) {
        for(i = 0; i < point_count && i < MULTITOUCH_MAX_POINTS; i++) {
            uint8_t id = track_id != NULL ? track_id[i] : i;
            uint32_t module_id;
            if(claimed[i]) {
                module_id = claimed_module_ids[i];
            } else {
                module_id = find_top_module((int32_t)x[i], (int32_t)y[i]);
                multitouch_module_t * view = find_module(module_id);
                if(view != NULL && view->module.kind == GAMEPAD_MODULE_VIEW_SLIDER &&
                   !view->view_motion.active && !released[(size_t)(view - s_modules)] && !track_was_present(id)) {
                    view->view_track_id = id;
                    (void)gamepad_view_motion_begin(&view->view_motion, x[i], y[i], now_ms);
                    view->view_last_log_ms = now_ms;
                    ESP_LOGI(TAG, "VIEW begin module=%lu id=%u x=%u y=%u",
                             (unsigned long)view->module.id,
                             (unsigned)id,
                             (unsigned)x[i],
                             (unsigned)y[i]);
                } else {
                    module_id = accumulate_point(&frame, (int32_t)x[i], (int32_t)y[i]);
                }
            }
            visual_points[accepted_count].x = x[i];
            visual_points[accepted_count].y = y[i];
            visual_points[accepted_count].track_id = track_id != NULL ? track_id[i] : i;
            visual_points[accepted_count].module_id = module_id;
            accepted_count++;
        }
    }

    publish_frame(&frame);
    if(s_visual_callback != NULL) {
        s_visual_callback(visual_points, accepted_count, s_visual_user_data);
    }
    s_previous_track_count = point_count < MULTITOUCH_MAX_POINTS ? point_count : MULTITOUCH_MAX_POINTS;
    for(i = 0; i < s_previous_track_count; i++) s_previous_track_ids[i] = track_id != NULL ? track_id[i] : i;
}

void gamepad_multitouch_input_on_points_xy(const uint16_t * x, const uint16_t * y, uint8_t point_count)
{
    gamepad_multitouch_input_on_points(x, y, NULL, point_count);
}
