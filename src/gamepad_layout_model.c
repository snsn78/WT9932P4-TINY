#include "gamepad_layout_model.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define GAMEPAD_LAYOUT_VERSION 1U
#define GAMEPAD_SLOT_COUNT 3U

#if defined(_WIN32)
#define GAMEPAD_PATH_SEP "\\"
#else
#define GAMEPAD_PATH_SEP "/"
#endif

static void apply_module_binding_defaults(gamepad_module_instance_t * module);
static bool action_matches_kind(gamepad_module_kind_t kind, gamepad_action_id_t action);

static const gamepad_module_catalog_entry_t g_catalog[] = {
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_A, "Button A", "A", 92, 92, 640, 240, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_B, "Button B", "B", 92, 92, 728, 176, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_X, "Button X", "X", 92, 92, 552, 176, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_Y, "Button Y", "Y", 92, 92, 640, 112, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_START, "Button Start", "START", 108, 76, 532, 126, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_SELECT, "Button Select", "SELECT", 108, 76, 384, 126, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_L1, "Button L1", "L1", 92, 92, 172, 32, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_L2, "Button L2", "L2", 92, 92, 172, 136, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_R1, "Button R1", "R1", 92, 92, 760, 32, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SINGLE, GAMEPAD_PRESET_R2, "Button R2", "R2", 92, 92, 760, 136, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT, GAMEPAD_PRESET_SHOULDER_LEFT_PAIR, "L1/L2 Pair", "L1/L2", 132, 176, GAMEPAD_DEFAULT_L_SHOULDER_X, GAMEPAD_DEFAULT_L_SHOULDER_Y, 80, 140 },
    { GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT, GAMEPAD_PRESET_SHOULDER_RIGHT_PAIR, "R1/R2 Pair", "R1/R2", 132, 176, GAMEPAD_DEFAULT_R_SHOULDER_X, GAMEPAD_DEFAULT_R_SHOULDER_Y, 80, 140 },
    { GAMEPAD_MODULE_JOYSTICK, GAMEPAD_PRESET_LEFT_STICK, "Left Stick", "L STICK", 252, 252, GAMEPAD_DEFAULT_LEFT_STICK_X, GAMEPAD_DEFAULT_LEFT_STICK_Y, 80, 140 },
    { GAMEPAD_MODULE_DPAD, GAMEPAD_PRESET_DPAD, "Dpad", "DPAD", 228, 228, GAMEPAD_DEFAULT_DPAD_X, GAMEPAD_DEFAULT_DPAD_Y, 80, 140 },
    { GAMEPAD_MODULE_VIEW_SLIDER, GAMEPAD_PRESET_VIEW_SLIDER, "View Slider 2D", "VIEW", 300, 340, GAMEPAD_DEFAULT_VIEW_SLIDER_X, GAMEPAD_DEFAULT_VIEW_SLIDER_Y, 80, 140 }
};

static const char * g_kind_names[] = {
    "BUTTON_SINGLE",
    "BUTTON_SHOULDER_LEFT",
    "BUTTON_SHOULDER_RIGHT",
    "JOYSTICK",
    "DPAD",
    "VIEW_SLIDER"
};

static const char * g_preset_names[] = {
    "A",
    "B",
    "X",
    "Y",
    "START",
    "SELECT",
    "L1",
    "L2",
    "R1",
    "R2",
    "LEFT_STICK",
    "DPAD",
    "VIEW_SLIDER",
    "SHOULDER_LEFT_PAIR",
    "SHOULDER_RIGHT_PAIR"
};

static const char * g_action_names[] = {
    "NONE",
    "MOVE",
    "LOOK",
    "DPAD",
    "BTN_A",
    "BTN_B",
    "BTN_X",
    "BTN_Y",
    "BTN_L1",
    "BTN_L2",
    "BTN_R1",
    "BTN_R2",
    "BTN_START",
    "BTN_SELECT"
};

static void set_error(gamepad_layout_error_t * err,
                      gamepad_layout_error_code_t code,
                      uint32_t module_id,
                      uint32_t other_id,
                      const char * text)
{
    if(err == NULL) return;
    err->code = code;
    err->module_id = module_id;
    err->other_module_id = other_id;
    lv_snprintf(err->message, sizeof(err->message), "%s", text ? text : "");
}

static bool rects_overlap(lv_coord_t ax, lv_coord_t ay, lv_coord_t aw, lv_coord_t ah,
                          lv_coord_t bx, lv_coord_t by, lv_coord_t bw, lv_coord_t bh)
{
    const lv_coord_t ar = ax + aw;
    const lv_coord_t ab = ay + ah;
    const lv_coord_t br = bx + bw;
    const lv_coord_t bb = by + bh;

    if(ar <= bx || br <= ax) return false;
    if(ab <= by || bb <= ay) return false;
    return true;
}

static bool ensure_directory_exists(const char * path)
{
#if defined(_WIN32)
    if(_mkdir(path) == 0 || errno == EEXIST) return true;
#else
    if(mkdir(path, 0755) == 0 || errno == EEXIST) return true;
#endif
    return false;
}

static bool ensure_config_dir(void)
{
#if defined(ESP_PLATFORM)
    return true;
#else
    char root_dir[128];
    const char * base = GAMEPAD_LAYOUT_CONFIG_DIR;
    const char * last_slash_back = strrchr(base, '\\');
    const char * last_slash_forward = strrchr(base, '/');
    const char * last_slash = last_slash_back;

    if(last_slash == NULL || (last_slash_forward != NULL && last_slash_forward > last_slash)) {
        last_slash = last_slash_forward;
    }

    if(last_slash == NULL) {
        return ensure_directory_exists(base);
    }

    lv_snprintf(root_dir, sizeof(root_dir), "%.*s", (int)(last_slash - base), base);
    if(root_dir[0] != '\0' && !ensure_directory_exists(root_dir)) {
        return false;
    }

    return ensure_directory_exists(base);
#endif
}

static void make_slot_path(uint8_t slot_id, char * path, size_t path_size)
{
#if defined(ESP_PLATFORM)
    lv_snprintf(path, path_size, "%s/gamepad_layout_slot_%u.json", GAMEPAD_LAYOUT_CONFIG_DIR, (unsigned)slot_id);
#else
    lv_snprintf(path, path_size, "%s%sslot_%u.json", GAMEPAD_LAYOUT_CONFIG_DIR, GAMEPAD_PATH_SEP, (unsigned)slot_id);
#endif
}

static void init_empty_config(gamepad_layout_config_t * cfg, uint8_t slot_id, const char * name)
{
    lv_memzero(cfg, sizeof(*cfg));
    cfg->version = GAMEPAD_LAYOUT_VERSION;
    cfg->slot_id = slot_id;
    cfg->is_active = false;
    cfg->next_id = 1U;
    lv_snprintf(cfg->name, sizeof(cfg->name), "%s", name);
}

static const gamepad_module_catalog_entry_t * catalog_find_by_labels(const char * kind_text,
                                                                     const char * preset_text)
{
    gamepad_module_kind_t kind;
    gamepad_preset_id_t preset;

    if(!gamepad_module_kind_from_string(kind_text, &kind)) return NULL;
    if(!gamepad_preset_id_from_string(preset_text, &preset)) return NULL;
    return gamepad_layout_catalog_find(kind, preset);
}

static bool add_module_direct(gamepad_layout_config_t * cfg, const gamepad_module_catalog_entry_t * entry)
{
    gamepad_module_instance_t * module;

    if(cfg->module_count >= GAMEPAD_LAYOUT_MAX_MODULES || entry == NULL) {
        return false;
    }

    module = &cfg->modules[cfg->module_count++];
    module->id = cfg->next_id++;
    module->kind = entry->kind;
    module->preset_id = entry->preset_id;
    module->x = entry->default_x;
    module->y = entry->default_y;
    module->w = entry->default_w;
    module->h = entry->default_h;
    module->visible = true;
    apply_module_binding_defaults(module);
    return true;
}

static const char * skip_ws(const char * p, const char * end)
{
    while(p < end && isspace((unsigned char)*p)) p++;
    return p;
}

static const char * find_key_in_range(const char * start, const char * end, const char * key)
{
    char needle[64];
    const char * p = start;

    lv_snprintf(needle, sizeof(needle), "\"%s\"", key);
    while((p = strstr(p, needle)) != NULL) {
        if(p < end) return p + strlen(needle);
        break;
    }
    return NULL;
}

static bool extract_value_span(const char * start,
                               const char * end,
                               const char * key,
                               const char ** value_start)
{
    const char * key_pos = find_key_in_range(start, end, key);
    const char * colon;

    if(key_pos == NULL) return false;
    colon = strchr(key_pos, ':');
    if(colon == NULL || colon >= end) return false;
    *value_start = skip_ws(colon + 1, end);
    return *value_start < end;
}

static bool json_read_int(const char * start, const char * end, const char * key, int * out_value)
{
    const char * value_start;
    char * tail = NULL;
    long value;

    if(!extract_value_span(start, end, key, &value_start)) return false;
    value = strtol(value_start, &tail, 10);
    if(tail == value_start || tail > end) return false;
    *out_value = (int)value;
    return true;
}

static bool json_read_bool(const char * start, const char * end, const char * key, bool * out_value)
{
    const char * value_start;

    if(!extract_value_span(start, end, key, &value_start)) return false;
    if((size_t)(end - value_start) >= 4U && strncmp(value_start, "true", 4) == 0) {
        *out_value = true;
        return true;
    }
    if((size_t)(end - value_start) >= 5U && strncmp(value_start, "false", 5) == 0) {
        *out_value = false;
        return true;
    }
    return false;
}

static bool json_read_string(const char * start,
                             const char * end,
                             const char * key,
                             char * out_text,
                             size_t out_size)
{
    const char * value_start;
    const char * value_end;
    size_t len;

    if(!extract_value_span(start, end, key, &value_start)) return false;
    if(*value_start != '"') return false;
    value_start++;
    value_end = strchr(value_start, '"');
    if(value_end == NULL || value_end > end) return false;

    len = (size_t)(value_end - value_start);
    if(len + 1U > out_size) len = out_size - 1U;
    memcpy(out_text, value_start, len);
    out_text[len] = '\0';
    return true;
}

static bool write_layout_file(const char * path, const gamepad_layout_config_t * cfg)
{
    FILE * fp;
    uint16_t i;

    fp = fopen(path, "wb");
    if(fp == NULL) {
        return false;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": %u,\n", (unsigned)cfg->version);
    fprintf(fp, "  \"slot_id\": %u,\n", (unsigned)cfg->slot_id);
    fprintf(fp, "  \"name\": \"%s\",\n", cfg->name);
    fprintf(fp, "  \"is_active\": %s,\n", cfg->is_active ? "true" : "false");
    fprintf(fp, "  \"next_id\": %u,\n", (unsigned)cfg->next_id);
    fprintf(fp, "  \"modules\": [\n");

    for(i = 0; i < cfg->module_count; i++) {
        const gamepad_module_instance_t * module = &cfg->modules[i];
        fprintf(fp,
                "    { \"id\": %u, \"kind\": \"%s\", \"preset_id\": \"%s\", \"x\": %d, \"y\": %d, \"w\": %d, \"h\": %d, \"visible\": %s, \"action_id\": \"%s\", \"locked\": %s, \"deadzone\": %d, \"sensitivity\": %d }%s\n",
                (unsigned)module->id,
                gamepad_module_kind_to_string(module->kind),
                gamepad_preset_id_to_string(module->preset_id),
                (int)module->x,
                (int)module->y,
                (int)module->w,
                (int)module->h,
                module->visible ? "true" : "false",
                gamepad_action_id_to_string(module->action_id),
                module->locked ? "true" : "false",
                (int)module->deadzone,
                (int)module->sensitivity,
                (i + 1U == cfg->module_count) ? "" : ",");
    }

    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
    return true;
}

static bool load_layout_file(const char * path, gamepad_layout_config_t * cfg)
{
    FILE * fp;
    long file_size;
    char * buffer = NULL;
    char kind_text[48];
    char preset_text[48];
    const char * modules_array;
    const char * array_end;
    const char * cursor;
    int int_value;

    fp = fopen(path, "rb");
    if(fp == NULL) return false;

    if(fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    file_size = ftell(fp);
    if(file_size <= 0) {
        fclose(fp);
        return false;
    }
    rewind(fp);

    buffer = (char *)lv_malloc((size_t)file_size + 1U);
    if(buffer == NULL) {
        fclose(fp);
        return false;
    }

    if(fread(buffer, 1U, (size_t)file_size, fp) != (size_t)file_size) {
        fclose(fp);
        lv_free(buffer);
        return false;
    }
    fclose(fp);
    buffer[file_size] = '\0';

    init_empty_config(cfg, 0, "Layout");

    if(json_read_int(buffer, buffer + file_size, "version", &int_value)) {
        cfg->version = (uint32_t)int_value;
    }
    if(!json_read_int(buffer, buffer + file_size, "slot_id", &int_value)) goto fail;
    cfg->slot_id = (uint8_t)int_value;
    if(!json_read_string(buffer, buffer + file_size, "name", cfg->name, sizeof(cfg->name))) goto fail;
    if(!json_read_bool(buffer, buffer + file_size, "is_active", &cfg->is_active)) goto fail;
    if(!json_read_int(buffer, buffer + file_size, "next_id", &int_value)) goto fail;
    cfg->next_id = (uint32_t)int_value;

    modules_array = find_key_in_range(buffer, buffer + file_size, "modules");
    if(modules_array == NULL) goto fail;
    modules_array = strchr(modules_array, '[');
    if(modules_array == NULL) goto fail;
    array_end = strchr(modules_array, ']');
    if(array_end == NULL) goto fail;
    cursor = modules_array;

    while((cursor = strchr(cursor, '{')) != NULL && cursor < array_end) {
        const char * object_end = strchr(cursor, '}');
        const gamepad_module_catalog_entry_t * entry;
        gamepad_module_instance_t * module;
        bool visible = true;

        if(object_end == NULL || object_end > array_end) goto fail;
        if(cfg->module_count >= GAMEPAD_LAYOUT_MAX_MODULES) goto fail;
        if(!json_read_int(cursor, object_end, "id", &int_value)) goto fail;
        if(!json_read_string(cursor, object_end, "kind", kind_text, sizeof(kind_text))) goto fail;
        if(!json_read_string(cursor, object_end, "preset_id", preset_text, sizeof(preset_text))) goto fail;
        entry = catalog_find_by_labels(kind_text, preset_text);
        if(entry == NULL) goto fail;

        module = &cfg->modules[cfg->module_count++];
        module->id = (uint32_t)int_value;
        module->kind = entry->kind;
        module->preset_id = entry->preset_id;

        if(!json_read_int(cursor, object_end, "x", &int_value)) goto fail;
        module->x = (lv_coord_t)int_value;
        if(!json_read_int(cursor, object_end, "y", &int_value)) goto fail;
        module->y = (lv_coord_t)int_value;
        if(!json_read_int(cursor, object_end, "w", &int_value)) goto fail;
        module->w = (lv_coord_t)int_value;
        if(!json_read_int(cursor, object_end, "h", &int_value)) goto fail;
        module->h = (lv_coord_t)int_value;
        if(json_read_bool(cursor, object_end, "visible", &visible)) {
            module->visible = visible;
        }
        else {
            module->visible = true;
        }

        /* ---- 新增绑定字段：旧 slot 无这些键时用默认值兜底(向后兼容) ---- */
        apply_module_binding_defaults(module);
        {
            char action_text[24];
            bool locked = false;
            int tmp_value;
            gamepad_action_id_t parsed_action;

            if(json_read_string(cursor, object_end, "action_id", action_text, sizeof(action_text)) &&
               gamepad_action_id_from_string(action_text, &parsed_action) &&
               action_matches_kind(module->kind, parsed_action)) {
                module->action_id = parsed_action;
            }
            /* action_id 缺失/非法/与 kind 不匹配时，保留 apply_module_binding_defaults 的默认绑定。 */
            if(json_read_bool(cursor, object_end, "locked", &locked)) {
                module->locked = locked;
            }
            if(json_read_int(cursor, object_end, "deadzone", &tmp_value)) {
                if(tmp_value < 0) tmp_value = 0;
                if(tmp_value > GAMEPAD_DEADZONE_MAX) tmp_value = GAMEPAD_DEADZONE_MAX;
                module->deadzone = (int16_t)tmp_value;
            }
            if(json_read_int(cursor, object_end, "sensitivity", &tmp_value)) {
                if(tmp_value < GAMEPAD_SENSITIVITY_MIN) tmp_value = GAMEPAD_SENSITIVITY_MIN;
                if(tmp_value > GAMEPAD_SENSITIVITY_MAX) tmp_value = GAMEPAD_SENSITIVITY_MAX;
                module->sensitivity = (int16_t)tmp_value;
            }
        }

        cursor = object_end + 1;
    }

    if(cfg->next_id == 0U) {
        cfg->next_id = 1U;
    }
    lv_free(buffer);
    return true;

fail:
    lv_free(buffer);
    return false;
}

static void sync_active_slot(uint8_t active_slot)
{
    uint8_t slot_id;

    for(slot_id = 1U; slot_id <= GAMEPAD_SLOT_COUNT; slot_id++) {
        char path[160];
        gamepad_layout_config_t cfg;

        make_slot_path(slot_id, path, sizeof(path));
        if(!load_layout_file(path, &cfg)) {
            continue;
        }

        cfg.is_active = (slot_id == active_slot);
        write_layout_file(path, &cfg);
    }
}

const gamepad_module_catalog_entry_t * gamepad_layout_catalog_get(size_t * count)
{
    if(count != NULL) {
        *count = sizeof(g_catalog) / sizeof(g_catalog[0]);
    }
    return g_catalog;
}

const gamepad_module_catalog_entry_t * gamepad_layout_catalog_find(gamepad_module_kind_t kind,
                                                                   gamepad_preset_id_t preset_id)
{
    size_t i;
    for(i = 0; i < sizeof(g_catalog) / sizeof(g_catalog[0]); i++) {
        if(g_catalog[i].kind == kind && g_catalog[i].preset_id == preset_id) {
            return &g_catalog[i];
        }
    }
    return NULL;
}

const char * gamepad_module_kind_to_string(gamepad_module_kind_t kind)
{
    if((unsigned)kind >= (sizeof(g_kind_names) / sizeof(g_kind_names[0]))) return "UNKNOWN";
    return g_kind_names[kind];
}

const char * gamepad_preset_id_to_string(gamepad_preset_id_t preset_id)
{
    if((unsigned)preset_id >= (sizeof(g_preset_names) / sizeof(g_preset_names[0]))) return "UNKNOWN";
    return g_preset_names[preset_id];
}

bool gamepad_preset_id_from_string(const char * text, gamepad_preset_id_t * preset_id)
{
    size_t i;
    for(i = 0; i < sizeof(g_preset_names) / sizeof(g_preset_names[0]); i++) {
        if(strcmp(text, g_preset_names[i]) == 0) {
            *preset_id = (gamepad_preset_id_t)i;
            return true;
        }
    }
    return false;
}

bool gamepad_module_kind_from_string(const char * text, gamepad_module_kind_t * kind)
{
    size_t i;
    for(i = 0; i < sizeof(g_kind_names) / sizeof(g_kind_names[0]); i++) {
        if(strcmp(text, g_kind_names[i]) == 0) {
            *kind = (gamepad_module_kind_t)i;
            return true;
        }
    }
    return false;
}

const char * gamepad_action_id_to_string(gamepad_action_id_t action_id)
{
    if((unsigned)action_id >= (sizeof(g_action_names) / sizeof(g_action_names[0]))) return "NONE";
    return g_action_names[action_id];
}

bool gamepad_action_id_from_string(const char * text, gamepad_action_id_t * action_id)
{
    size_t i;
    for(i = 0; i < sizeof(g_action_names) / sizeof(g_action_names[0]); i++) {
        if(strcmp(text, g_action_names[i]) == 0) {
            *action_id = (gamepad_action_id_t)i;
            return true;
        }
    }
    return false;
}

gamepad_action_id_t gamepad_preset_default_action(gamepad_preset_id_t preset_id)
{
    switch(preset_id) {
        case GAMEPAD_PRESET_A:      return GAMEPAD_ACTION_BTN_A;
        case GAMEPAD_PRESET_B:      return GAMEPAD_ACTION_BTN_B;
        case GAMEPAD_PRESET_X:      return GAMEPAD_ACTION_BTN_X;
        case GAMEPAD_PRESET_Y:      return GAMEPAD_ACTION_BTN_Y;
        case GAMEPAD_PRESET_START:  return GAMEPAD_ACTION_BTN_START;
        case GAMEPAD_PRESET_SELECT: return GAMEPAD_ACTION_BTN_SELECT;
        case GAMEPAD_PRESET_L1:     return GAMEPAD_ACTION_BTN_L1;
        case GAMEPAD_PRESET_L2:     return GAMEPAD_ACTION_BTN_L2;
        case GAMEPAD_PRESET_R1:     return GAMEPAD_ACTION_BTN_R1;
        case GAMEPAD_PRESET_R2:     return GAMEPAD_ACTION_BTN_R2;
        case GAMEPAD_PRESET_LEFT_STICK:         return GAMEPAD_ACTION_MOVE;
        case GAMEPAD_PRESET_DPAD:               return GAMEPAD_ACTION_DPAD;
        case GAMEPAD_PRESET_VIEW_SLIDER:        return GAMEPAD_ACTION_LOOK;
        case GAMEPAD_PRESET_SHOULDER_LEFT_PAIR: return GAMEPAD_ACTION_BTN_L1;
        case GAMEPAD_PRESET_SHOULDER_RIGHT_PAIR: return GAMEPAD_ACTION_BTN_R1;
        default: return GAMEPAD_ACTION_NONE;
    }
}

bool gamepad_action_is_remappable(gamepad_module_kind_t kind)
{
    /* 单按钮、摇杆、视角滑块可自由改绑；DPad 与肩键对为固定复合输入。 */
    switch(kind) {
        case GAMEPAD_MODULE_BUTTON_SINGLE:
        case GAMEPAD_MODULE_JOYSTICK:
        case GAMEPAD_MODULE_VIEW_SLIDER:
            return true;
        default:
            return false;
    }
}

static int16_t default_deadzone_for_kind(gamepad_module_kind_t kind)
{
    /* 模拟类默认给一点死区，离散类无死区。 */
    if(kind == GAMEPAD_MODULE_JOYSTICK || kind == GAMEPAD_MODULE_VIEW_SLIDER) {
        return GAMEPAD_DEADZONE_ANALOG_DEFAULT;
    }
    return 0;
}

/* 为新建模块实例填入默认绑定/参数(action_id/locked/deadzone/sensitivity)。 */
static void apply_module_binding_defaults(gamepad_module_instance_t * module)
{
    module->action_id = gamepad_preset_default_action(module->preset_id);
    module->locked = false;
    module->deadzone = default_deadzone_for_kind(module->kind);
    module->sensitivity = GAMEPAD_SENSITIVITY_DEFAULT;
}

/* 校验 action 是否与模块 kind 匹配(用于加载时对外部/旧数据做边界收口)。 */
static bool action_matches_kind(gamepad_module_kind_t kind, gamepad_action_id_t action)
{
    switch(kind) {
        case GAMEPAD_MODULE_JOYSTICK:
        case GAMEPAD_MODULE_VIEW_SLIDER:
            return action == GAMEPAD_ACTION_MOVE ||
                   action == GAMEPAD_ACTION_LOOK ||
                   action == GAMEPAD_ACTION_NONE;
        case GAMEPAD_MODULE_BUTTON_SINGLE:
            return (action >= GAMEPAD_ACTION_BTN_A && action <= GAMEPAD_ACTION_BTN_SELECT) ||
                   action == GAMEPAD_ACTION_NONE;
        case GAMEPAD_MODULE_DPAD:
            return action == GAMEPAD_ACTION_DPAD;
        case GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT:
        case GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT:
            /* 肩键对固定为其默认绑定 */
            return action == gamepad_preset_default_action(
                (kind == GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT)
                    ? GAMEPAD_PRESET_SHOULDER_LEFT_PAIR
                    : GAMEPAD_PRESET_SHOULDER_RIGHT_PAIR);
        default:
            return false;
    }
}

void gamepad_layout_load_default(gamepad_layout_config_t * cfg)
{
    const gamepad_module_catalog_entry_t * entry;

    init_empty_config(cfg, 0U, "Working Layout");

    entry = gamepad_layout_catalog_find(GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT, GAMEPAD_PRESET_SHOULDER_LEFT_PAIR);
    add_module_direct(cfg, entry);
    entry = gamepad_layout_catalog_find(GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT, GAMEPAD_PRESET_SHOULDER_RIGHT_PAIR);
    add_module_direct(cfg, entry);
    entry = gamepad_layout_catalog_find(GAMEPAD_MODULE_DPAD, GAMEPAD_PRESET_DPAD);
    add_module_direct(cfg, entry);
    entry = gamepad_layout_catalog_find(GAMEPAD_MODULE_JOYSTICK, GAMEPAD_PRESET_LEFT_STICK);
    add_module_direct(cfg, entry);
    entry = gamepad_layout_catalog_find(GAMEPAD_MODULE_VIEW_SLIDER, GAMEPAD_PRESET_VIEW_SLIDER);
    add_module_direct(cfg, entry);
}

bool gamepad_layout_can_place(const gamepad_layout_config_t * cfg,
                              uint32_t ignored_id,
                              lv_coord_t x,
                              lv_coord_t y,
                              lv_coord_t w,
                              lv_coord_t h,
                              gamepad_layout_error_t * err)
{
    uint16_t i;

    if(x < 0 || y < 0 || w <= 0 || h <= 0 ||
       (x + w) > GAMEPAD_LAYOUT_SCREEN_W || (y + h) > GAMEPAD_LAYOUT_SCREEN_H) {
        set_error(err, GAMEPAD_LAYOUT_ERROR_OUT_OF_BOUNDS, ignored_id, 0U, "Module must stay inside the screen.");
        return false;
    }

    for(i = 0; i < cfg->module_count; i++) {
        const gamepad_module_instance_t * other = &cfg->modules[i];

        if(!other->visible || other->id == ignored_id) continue;
        if(rects_overlap(x, y, w, h, other->x, other->y, other->w, other->h)) {
            set_error(err, GAMEPAD_LAYOUT_ERROR_OVERLAP, ignored_id, other->id, "Modules cannot overlap.");
            return false;
        }
    }

    set_error(err, GAMEPAD_LAYOUT_ERROR_NONE, 0U, 0U, "");
    return true;
}

bool gamepad_layout_validate(const gamepad_layout_config_t * cfg, gamepad_layout_error_t * err)
{
    uint16_t i;

    for(i = 0; i < cfg->module_count; i++) {
        const gamepad_module_instance_t * module = &cfg->modules[i];

        if(!module->visible) continue;
        if(!gamepad_layout_can_place(cfg, module->id, module->x, module->y, module->w, module->h, err)) {
            if(err != NULL && err->module_id == 0U) {
                err->module_id = module->id;
            }
            return false;
        }
    }

    set_error(err, GAMEPAD_LAYOUT_ERROR_NONE, 0U, 0U, "");
    return true;
}

bool gamepad_layout_try_add_module(gamepad_layout_config_t * cfg,
                                   gamepad_module_kind_t kind,
                                   gamepad_preset_id_t preset_id,
                                   gamepad_layout_error_t * err)
{
    const gamepad_module_catalog_entry_t * entry = gamepad_layout_catalog_find(kind, preset_id);
    lv_coord_t best_x = 0;
    lv_coord_t best_y = 0;
    int32_t best_score = INT32_MAX;
    bool found = false;
    lv_coord_t x;
    lv_coord_t y;

    if(cfg->module_count >= GAMEPAD_LAYOUT_MAX_MODULES) {
        set_error(err, GAMEPAD_LAYOUT_ERROR_MAX_MODULES, 0U, 0U, "Module limit reached.");
        return false;
    }
    if(entry == NULL) {
        set_error(err, GAMEPAD_LAYOUT_ERROR_INVALID_DATA, 0U, 0U, "Unknown module preset.");
        return false;
    }

    if(gamepad_layout_can_place(cfg, 0U, entry->default_x, entry->default_y, entry->default_w, entry->default_h, NULL)) {
        best_x = entry->default_x;
        best_y = entry->default_y;
        found = true;
    }
    else {
        for(y = 0; y <= (GAMEPAD_LAYOUT_SCREEN_H - entry->default_h); y += GAMEPAD_LAYOUT_GRID_STEP) {
            for(x = 0; x <= (GAMEPAD_LAYOUT_SCREEN_W - entry->default_w); x += GAMEPAD_LAYOUT_GRID_STEP) {
                const int32_t score = LV_ABS(x - entry->default_x) + LV_ABS(y - entry->default_y);
                if(!gamepad_layout_can_place(cfg, 0U, x, y, entry->default_w, entry->default_h, NULL)) {
                    continue;
                }
                if(score < best_score) {
                    best_score = score;
                    best_x = x;
                    best_y = y;
                    found = true;
                }
            }
        }
    }

    if(!found) {
        set_error(err, GAMEPAD_LAYOUT_ERROR_OVERLAP, 0U, 0U, "No legal region is available for this module.");
        return false;
    }

    cfg->modules[cfg->module_count].id = cfg->next_id++;
    cfg->modules[cfg->module_count].kind = entry->kind;
    cfg->modules[cfg->module_count].preset_id = entry->preset_id;
    cfg->modules[cfg->module_count].x = best_x;
    cfg->modules[cfg->module_count].y = best_y;
    cfg->modules[cfg->module_count].w = entry->default_w;
    cfg->modules[cfg->module_count].h = entry->default_h;
    cfg->modules[cfg->module_count].visible = true;
    apply_module_binding_defaults(&cfg->modules[cfg->module_count]);
    cfg->module_count++;

    set_error(err, GAMEPAD_LAYOUT_ERROR_NONE, 0U, 0U, "");
    return true;
}

bool gamepad_layout_remove_module(gamepad_layout_config_t * cfg, uint32_t module_id)
{
    uint16_t i;

    for(i = 0; i < cfg->module_count; i++) {
        if(cfg->modules[i].id != module_id) continue;
        if(i + 1U < cfg->module_count) {
            memmove(&cfg->modules[i], &cfg->modules[i + 1U],
                    (size_t)(cfg->module_count - i - 1U) * sizeof(cfg->modules[0]));
        }
        cfg->module_count--;
        return true;
    }
    return false;
}

gamepad_module_instance_t * gamepad_layout_find_module(gamepad_layout_config_t * cfg, uint32_t module_id)
{
    uint16_t i;
    for(i = 0; i < cfg->module_count; i++) {
        if(cfg->modules[i].id == module_id) {
            return &cfg->modules[i];
        }
    }
    return NULL;
}

const gamepad_module_instance_t * gamepad_layout_find_module_const(const gamepad_layout_config_t * cfg,
                                                                   uint32_t module_id)
{
    uint16_t i;
    for(i = 0; i < cfg->module_count; i++) {
        if(cfg->modules[i].id == module_id) {
            return &cfg->modules[i];
        }
    }
    return NULL;
}

bool gamepad_layout_save_slot(uint8_t slot_id, const gamepad_layout_config_t * cfg)
{
    char path[160];
    gamepad_layout_config_t copy;

    if(slot_id == 0U || slot_id > GAMEPAD_SLOT_COUNT) return false;
    if(!ensure_config_dir()) return false;

    copy = *cfg;
    copy.slot_id = slot_id;
    copy.is_active = true;
    if(copy.name[0] == '\0') {
        lv_snprintf(copy.name, sizeof(copy.name), "Layout %u", (unsigned)slot_id);
    }

    make_slot_path(slot_id, path, sizeof(path));
    if(!write_layout_file(path, &copy)) {
        return false;
    }

    sync_active_slot(slot_id);
    return true;
}

bool gamepad_layout_load_slot(uint8_t slot_id, gamepad_layout_config_t * cfg)
{
    char path[160];

    if(slot_id == 0U || slot_id > GAMEPAD_SLOT_COUNT) return false;
    make_slot_path(slot_id, path, sizeof(path));
    if(!load_layout_file(path, cfg)) {
        return false;
    }

    cfg->slot_id = slot_id;
    cfg->is_active = true;
    sync_active_slot(slot_id);
    return true;
}

bool gamepad_layout_load_initial(gamepad_layout_config_t * cfg, uint8_t * slot_id)
{
    uint8_t first_existing = 0U;
    uint8_t i;

    for(i = 1U; i <= GAMEPAD_SLOT_COUNT; i++) {
        char path[160];
        gamepad_layout_config_t candidate;

        make_slot_path(i, path, sizeof(path));
        if(!load_layout_file(path, &candidate)) {
            continue;
        }

        if(first_existing == 0U) {
            first_existing = i;
        }
        if(candidate.is_active) {
            *cfg = candidate;
            if(slot_id != NULL) *slot_id = i;
            return true;
        }
    }

    if(first_existing != 0U) {
        char path[160];
        make_slot_path(first_existing, path, sizeof(path));
        if(load_layout_file(path, cfg)) {
            if(slot_id != NULL) *slot_id = first_existing;
            return true;
        }
    }

    gamepad_layout_load_default(cfg);
    if(slot_id != NULL) *slot_id = 0U;
    return false;
}

bool gamepad_layout_get_slot_summary(uint8_t slot_id, gamepad_layout_slot_summary_t * summary)
{
    char path[160];
    gamepad_layout_config_t cfg;

    if(summary == NULL) return false;
    lv_memzero(summary, sizeof(*summary));
    summary->slot_id = slot_id;

    if(slot_id == 0U || slot_id > GAMEPAD_SLOT_COUNT) {
        return false;
    }

    make_slot_path(slot_id, path, sizeof(path));

    if(!load_layout_file(path, &cfg)) {
        summary->exists = false;
        summary->valid = false;
        lv_snprintf(summary->name, sizeof(summary->name), "Empty");
        return false;
    }

    summary->exists = true;
    summary->valid = gamepad_layout_validate(&cfg, NULL);
    summary->is_active = cfg.is_active;
    summary->module_count = cfg.module_count;
    lv_snprintf(summary->name, sizeof(summary->name), "%s", cfg.name);
    return true;
}
