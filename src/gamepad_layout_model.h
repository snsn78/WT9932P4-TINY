#ifndef GAMEPAD_LAYOUT_MODEL_H
#define GAMEPAD_LAYOUT_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"

#define GAMEPAD_LAYOUT_SCREEN_W 1024
#define GAMEPAD_LAYOUT_SCREEN_H 600
#define GAMEPAD_LAYOUT_GRID_STEP 16
#define GAMEPAD_LAYOUT_MAX_MODULES 32
#if defined(ESP_PLATFORM)
#include "bsp/esp-bsp.h"
#define GAMEPAD_LAYOUT_CONFIG_DIR BSP_SPIFFS_MOUNT_POINT
#else
#define GAMEPAD_LAYOUT_CONFIG_DIR "configs/gamepad_layouts"
#endif

/* ===== 后续可自行调整：7 寸 1024x600 默认布局坐标 ===== */
#define GAMEPAD_DEFAULT_DPAD_X               40   /* 默认十字键 X */
#define GAMEPAD_DEFAULT_DPAD_Y              150   /* 默认十字键 Y */
#define GAMEPAD_DEFAULT_LEFT_STICK_X         36   /* 默认左摇杆 X */
#define GAMEPAD_DEFAULT_LEFT_STICK_Y        342   /* 默认左摇杆 Y */
#define GAMEPAD_DEFAULT_VIEW_SLIDER_X       696   /* 默认二维视角滑块 X */
#define GAMEPAD_DEFAULT_VIEW_SLIDER_Y       236   /* 默认二维视角滑块 Y */
#define GAMEPAD_DEFAULT_L_SHOULDER_X         28   /* 默认 L1/L2 肩键组 X */
#define GAMEPAD_DEFAULT_L_SHOULDER_Y         32   /* 默认 L1/L2 肩键组 Y */
#define GAMEPAD_DEFAULT_R_SHOULDER_X        864   /* 默认 R1/R2 肩键组 X */
#define GAMEPAD_DEFAULT_R_SHOULDER_Y         32   /* 默认 R1/R2 肩键组 Y */

typedef enum {
    GAMEPAD_MODULE_BUTTON_SINGLE = 0,
    GAMEPAD_MODULE_BUTTON_SHOULDER_LEFT,
    GAMEPAD_MODULE_BUTTON_SHOULDER_RIGHT,
    GAMEPAD_MODULE_JOYSTICK,
    GAMEPAD_MODULE_DPAD,
    GAMEPAD_MODULE_VIEW_SLIDER
} gamepad_module_kind_t;

typedef enum {
    GAMEPAD_PRESET_A = 0,
    GAMEPAD_PRESET_B,
    GAMEPAD_PRESET_X,
    GAMEPAD_PRESET_Y,
    GAMEPAD_PRESET_START,
    GAMEPAD_PRESET_SELECT,
    GAMEPAD_PRESET_L1,
    GAMEPAD_PRESET_L2,
    GAMEPAD_PRESET_R1,
    GAMEPAD_PRESET_R2,
    GAMEPAD_PRESET_LEFT_STICK,
    GAMEPAD_PRESET_DPAD,
    GAMEPAD_PRESET_VIEW_SLIDER,
    GAMEPAD_PRESET_SHOULDER_LEFT_PAIR,
    GAMEPAD_PRESET_SHOULDER_RIGHT_PAIR
} gamepad_preset_id_t;

/* 运行时动作绑定 (Step 9)：描述“这个模块控制什么动作”。
 * 与控件外观解耦，供动作映射层(gamepad_action_mapper)使用。 */
typedef enum {
    GAMEPAD_ACTION_NONE = 0,
    GAMEPAD_ACTION_MOVE,        /* 模拟：左摇杆移动 -> move_x/move_y */
    GAMEPAD_ACTION_LOOK,        /* 模拟：视角 -> look_x/look_y */
    GAMEPAD_ACTION_DPAD,        /* 离散：四方向 -> dpad_* */
    GAMEPAD_ACTION_BTN_A,
    GAMEPAD_ACTION_BTN_B,
    GAMEPAD_ACTION_BTN_X,
    GAMEPAD_ACTION_BTN_Y,
    GAMEPAD_ACTION_BTN_L1,      /* 肩键对：channel 0 -> L1，channel 1 -> L2 */
    GAMEPAD_ACTION_BTN_L2,
    GAMEPAD_ACTION_BTN_R1,      /* 肩键对：channel 0 -> R1，channel 1 -> R2 */
    GAMEPAD_ACTION_BTN_R2,
    GAMEPAD_ACTION_BTN_START,
    GAMEPAD_ACTION_BTN_SELECT,
    GAMEPAD_ACTION_COUNT
} gamepad_action_id_t;

/* deadzone / sensitivity 以整型百分比存储，便于手写 JSON 序列化。
 * deadzone: 死区，0..90(%)。sensitivity: 灵敏度，10..400(%)，100 表示 1.0 倍。 */
#define GAMEPAD_SENSITIVITY_DEFAULT 100
#define GAMEPAD_SENSITIVITY_MIN     10
#define GAMEPAD_SENSITIVITY_MAX     400
#define GAMEPAD_DEADZONE_MAX        90
#define GAMEPAD_DEADZONE_ANALOG_DEFAULT 8

typedef struct {
    uint32_t id;
    gamepad_module_kind_t kind;
    gamepad_preset_id_t preset_id;
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t w;
    lv_coord_t h;
    bool visible;
    gamepad_action_id_t action_id;  /* 该模块绑定的动作 */
    bool locked;                    /* 锁定：编辑模式下禁止移动/缩放/改绑定 */
    int16_t deadzone;               /* 死区百分比 0..90 */
    int16_t sensitivity;            /* 灵敏度百分比，100 = 1.0 倍 */
} gamepad_module_instance_t;

typedef struct {
    uint32_t version;
    uint8_t slot_id;
    bool is_active;
    char name[32];
    uint32_t next_id;
    uint16_t module_count;
    gamepad_module_instance_t modules[GAMEPAD_LAYOUT_MAX_MODULES];
} gamepad_layout_config_t;

typedef enum {
    GAMEPAD_LAYOUT_ERROR_NONE = 0,
    GAMEPAD_LAYOUT_ERROR_OUT_OF_BOUNDS,
    GAMEPAD_LAYOUT_ERROR_OVERLAP,
    GAMEPAD_LAYOUT_ERROR_MAX_MODULES,
    GAMEPAD_LAYOUT_ERROR_INVALID_DATA
} gamepad_layout_error_code_t;

typedef struct {
    gamepad_layout_error_code_t code;
    uint32_t module_id;
    uint32_t other_module_id;
    char message[128];
} gamepad_layout_error_t;

typedef struct {
    gamepad_module_kind_t kind;
    gamepad_preset_id_t preset_id;
    const char * library_label;
    const char * output_label;
    lv_coord_t default_w;
    lv_coord_t default_h;
    lv_coord_t default_x;
    lv_coord_t default_y;
    uint16_t min_scale_pct;
    uint16_t max_scale_pct;
} gamepad_module_catalog_entry_t;

typedef struct {
    bool exists;
    bool valid;
    bool is_active;
    uint8_t slot_id;
    uint16_t module_count;
    char name[32];
} gamepad_layout_slot_summary_t;

const gamepad_module_catalog_entry_t * gamepad_layout_catalog_get(size_t * count);
const gamepad_module_catalog_entry_t * gamepad_layout_catalog_find(gamepad_module_kind_t kind,
                                                                   gamepad_preset_id_t preset_id);
const char * gamepad_module_kind_to_string(gamepad_module_kind_t kind);
const char * gamepad_preset_id_to_string(gamepad_preset_id_t preset_id);
bool gamepad_preset_id_from_string(const char * text, gamepad_preset_id_t * preset_id);
bool gamepad_module_kind_from_string(const char * text, gamepad_module_kind_t * kind);

const char * gamepad_action_id_to_string(gamepad_action_id_t action_id);
bool gamepad_action_id_from_string(const char * text, gamepad_action_id_t * action_id);
/* 该预设的默认动作绑定(旧 slot 无 action_id 字段时的兜底值)。 */
gamepad_action_id_t gamepad_preset_default_action(gamepad_preset_id_t preset_id);
/* 该动作是否允许在编辑器里自由改绑(单按钮/摇杆/滑块可改；DPad/肩键对固定)。 */
bool gamepad_action_is_remappable(gamepad_module_kind_t kind);

void gamepad_layout_load_default(gamepad_layout_config_t * cfg);
bool gamepad_layout_validate(const gamepad_layout_config_t * cfg, gamepad_layout_error_t * err);
bool gamepad_layout_can_place(const gamepad_layout_config_t * cfg,
                              uint32_t ignored_id,
                              lv_coord_t x,
                              lv_coord_t y,
                              lv_coord_t w,
                              lv_coord_t h,
                              gamepad_layout_error_t * err);
bool gamepad_layout_try_add_module(gamepad_layout_config_t * cfg,
                                   gamepad_module_kind_t kind,
                                   gamepad_preset_id_t preset_id,
                                   gamepad_layout_error_t * err);
bool gamepad_layout_remove_module(gamepad_layout_config_t * cfg, uint32_t module_id);
gamepad_module_instance_t * gamepad_layout_find_module(gamepad_layout_config_t * cfg, uint32_t module_id);
const gamepad_module_instance_t * gamepad_layout_find_module_const(const gamepad_layout_config_t * cfg,
                                                                   uint32_t module_id);

bool gamepad_layout_save_slot(uint8_t slot_id, const gamepad_layout_config_t * cfg);
bool gamepad_layout_load_slot(uint8_t slot_id, gamepad_layout_config_t * cfg);
bool gamepad_layout_load_initial(gamepad_layout_config_t * cfg, uint8_t * slot_id);
bool gamepad_layout_get_slot_summary(uint8_t slot_id, gamepad_layout_slot_summary_t * summary);

#endif
