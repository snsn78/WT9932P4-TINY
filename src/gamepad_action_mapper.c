#include "gamepad_action_mapper.h"

#include <math.h>
#include <stddef.h>   /* NULL */

/* 运行时动作映射层实现 (Step 10) */

typedef struct {
    lv_obj_t * root;
    gamepad_action_binding_t binding;
    bool used;
} mapper_entry_t;

/* 表容量与单个布局最大模块数一致即可。 */
static mapper_entry_t g_entries[GAMEPAD_LAYOUT_MAX_MODULES];

void gamepad_action_mapper_reset(void)
{
    for(size_t i = 0; i < (sizeof(g_entries) / sizeof(g_entries[0])); i++) {
        g_entries[i].root = NULL;
        g_entries[i].used = false;
    }
}

void gamepad_action_mapper_register(lv_obj_t * root, const gamepad_module_instance_t * module)
{
    size_t i;

    if(root == NULL || module == NULL) return;

    for(i = 0; i < (sizeof(g_entries) / sizeof(g_entries[0])); i++) {
        if(g_entries[i].used) continue;
        g_entries[i].root = root;
        g_entries[i].binding.action_id = module->action_id;
        g_entries[i].binding.kind = module->kind;
        g_entries[i].binding.preset_id = module->preset_id;
        g_entries[i].binding.deadzone = module->deadzone;
        g_entries[i].binding.sensitivity = module->sensitivity;
        g_entries[i].used = true;
        return;
    }
    /* 表满则丢弃：不影响外观，仅该控件无法产生输入。 */
}

const gamepad_action_binding_t * gamepad_action_mapper_find(lv_obj_t * child)
{
    lv_obj_t * node = child;

    while(node != NULL) {
        size_t i;
        for(i = 0; i < (sizeof(g_entries) / sizeof(g_entries[0])); i++) {
            if(g_entries[i].used && g_entries[i].root == node) {
                return &g_entries[i].binding;
            }
        }
        node = lv_obj_get_parent(node);
    }
    return NULL;
}

/* 径向死区 + 灵敏度：向量长度小于死区则清零，否则重映射到 [0,1] 再乘灵敏度。 */
static void apply_deadzone_sensitivity(const gamepad_action_binding_t * b,
                                       float nx, float ny,
                                       float * out_x, float * out_y)
{
    float dz = (float)b->deadzone / 100.0f;
    float sens = (float)b->sensitivity / 100.0f;
    float mag;

    if(dz < 0.0f) dz = 0.0f;
    if(dz > 0.95f) dz = 0.95f;

    mag = sqrtf(nx * nx + ny * ny);

    if(mag <= dz || mag <= 1e-6f) {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }

    /* 把 [dz,1] 线性拉伸回 [0,1]，保持方向。 */
    {
        float scaled = (mag - dz) / (1.0f - dz);
        float factor = (scaled / mag) * sens;
        *out_x = nx * factor;
        *out_y = ny * factor;
    }
}

void gamepad_action_mapper_report_analog(const gamepad_action_binding_t * binding, float nx, float ny)
{
    float ox;
    float oy;

    if(binding == NULL) return;

    apply_deadzone_sensitivity(binding, nx, ny, &ox, &oy);

    switch(binding->action_id) {
        case GAMEPAD_ACTION_MOVE:
            gamepad_input_set_move(ox, oy);
            break;
        case GAMEPAD_ACTION_LOOK:
            gamepad_input_set_look(ox, oy);
            break;
        default:
            /* 该绑定不接收模拟量(如误把按钮绑给了摇杆)：忽略。 */
            break;
    }
}

/* 把“单按钮类动作”解析成具体 button id。channel 用于肩键对的第二键。 */
static bool resolve_button(gamepad_action_id_t action, uint8_t channel, gamepad_button_id_t * out)
{
    switch(action) {
        case GAMEPAD_ACTION_BTN_A:      *out = GAMEPAD_BTN_A; return true;
        case GAMEPAD_ACTION_BTN_B:      *out = GAMEPAD_BTN_B; return true;
        case GAMEPAD_ACTION_BTN_X:      *out = GAMEPAD_BTN_X; return true;
        case GAMEPAD_ACTION_BTN_Y:      *out = GAMEPAD_BTN_Y; return true;
        case GAMEPAD_ACTION_BTN_START:  *out = GAMEPAD_BTN_START; return true;
        case GAMEPAD_ACTION_BTN_SELECT: *out = GAMEPAD_BTN_SELECT; return true;
        case GAMEPAD_ACTION_BTN_L1:
            /* 肩键对：channel 0 -> L1，channel 1 -> L2 */
            *out = (channel == GAMEPAD_CHANNEL_TRIGGER) ? GAMEPAD_BTN_L2 : GAMEPAD_BTN_L1;
            return true;
        case GAMEPAD_ACTION_BTN_L2:     *out = GAMEPAD_BTN_L2; return true;
        case GAMEPAD_ACTION_BTN_R1:
            *out = (channel == GAMEPAD_CHANNEL_TRIGGER) ? GAMEPAD_BTN_R2 : GAMEPAD_BTN_R1;
            return true;
        case GAMEPAD_ACTION_BTN_R2:     *out = GAMEPAD_BTN_R2; return true;
        default:
            return false;
    }
}

void gamepad_action_mapper_report_button(const gamepad_action_binding_t * binding, uint8_t channel, bool pressed)
{
    gamepad_button_id_t button;

    if(binding == NULL) return;

    if(binding->action_id == GAMEPAD_ACTION_DPAD) {
        if(channel < GAMEPAD_DPAD_COUNT) {
            gamepad_input_set_dpad((gamepad_dpad_dir_t)channel, pressed);
        }
        return;
    }

    if(resolve_button(binding->action_id, channel, &button)) {
        gamepad_input_set_button(button, pressed);
    }
    /* NONE / MOVE / LOOK 不接收离散按下：忽略。 */
}
