#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "gamepad_input_state.h"
#include "gamepad_multitouch_input.h"

static gamepad_multitouch_visual_point_t visual_points[5];
static uint8_t visual_point_count;

static void capture_visual_frame(const gamepad_multitouch_visual_point_t * points,
                                 uint8_t point_count,
                                 void * user_data)
{
    (void)user_data;
    visual_point_count = point_count;
    for(uint8_t i = 0; i < point_count; i++) visual_points[i] = points[i];
}

static gamepad_module_instance_t make_button(uint32_t id,
                                             gamepad_action_id_t action,
                                             int32_t x,
                                             int32_t y)
{
    gamepad_module_instance_t module = {0};
    module.id = id;
    module.kind = GAMEPAD_MODULE_BUTTON_SINGLE;
    module.x = x;
    module.y = y;
    module.w = 100;
    module.h = 100;
    module.visible = true;
    module.action_id = action;
    module.sensitivity = GAMEPAD_SENSITIVITY_DEFAULT;
    return module;
}

static gamepad_module_instance_t make_analog(uint32_t id,
                                             gamepad_module_kind_t kind,
                                             gamepad_action_id_t action,
                                             int32_t x,
                                             int32_t y,
                                             int32_t w,
                                             int32_t h)
{
    gamepad_module_instance_t module = {0};
    module.id = id;
    module.kind = kind;
    module.x = x;
    module.y = y;
    module.w = w;
    module.h = h;
    module.visible = true;
    module.action_id = action;
    module.sensitivity = GAMEPAD_SENSITIVITY_DEFAULT;
    return module;
}

static void test_view_track_lock_and_release(void)
{
    const gamepad_module_instance_t button = make_button(1, GAMEPAD_ACTION_BTN_A, 20, 20);
    const gamepad_module_instance_t view = make_analog(2,
                                                       GAMEPAD_MODULE_VIEW_SLIDER,
                                                       GAMEPAD_ACTION_LOOK,
                                                       500,
                                                       20,
                                                       400,
                                                       300);
    const gamepad_input_state_t * state;
    const uint16_t first_x[] = {600, 50};
    const uint16_t first_y[] = {100, 50};
    const uint8_t first_ids[] = {7, 2};
    const uint16_t reordered_x[] = {50, 608};
    const uint16_t reordered_y[] = {50, 100};
    const uint8_t reordered_ids[] = {2, 7};
    const uint16_t two_view_x[] = {616, 700};
    const uint16_t two_view_y[] = {100, 120};
    const uint8_t two_view_ids[] = {7, 9};
    const uint16_t held_x[] = {720};
    const uint16_t held_y[] = {120};
    const uint8_t held_ids[] = {9};

    gamepad_multitouch_input_reset();
    gamepad_multitouch_input_register_module(&button);
    gamepad_multitouch_input_register_module(&view);

    gamepad_multitouch_input_on_points_at_time(first_x, first_y, first_ids, 2, 100U);
    state = gamepad_input_get_state();
    assert(state->look_x == 0.0f);
    assert(state->look_y == 0.0f);
    assert(state->btn_a);

    gamepad_multitouch_input_on_points_at_time(reordered_x, reordered_y, reordered_ids, 2, 108U);
    state = gamepad_input_get_state();
    assert(state->look_x > 0.0f);
    assert(state->btn_a);
    assert(visual_point_count == 2);
    assert(visual_points[1].track_id == 7);
    assert(visual_points[1].module_id == 2U);

    gamepad_multitouch_input_on_points_at_time(two_view_x, two_view_y, two_view_ids, 2, 116U);
    assert(gamepad_input_get_state()->look_x > 0.0f);

    gamepad_multitouch_input_on_points_at_time(held_x, held_y, held_ids, 1, 124U);
    assert(gamepad_input_get_state()->look_x == 0.0f);

    gamepad_multitouch_input_on_points_at_time(held_x, held_y, held_ids, 1, 132U);
    assert(gamepad_input_get_state()->look_x == 0.0f);

    gamepad_multitouch_input_on_points_at_time(NULL, NULL, NULL, 0, 140U);
    assert(gamepad_input_get_state()->look_x == 0.0f);

    gamepad_multitouch_input_on_points_at_time(held_x, held_y, held_ids, 1, 148U);
    assert(gamepad_input_get_state()->look_x == 0.0f);
    gamepad_multitouch_input_on_points_at_time(NULL, NULL, NULL, 0, 156U);
    assert(gamepad_input_get_state()->look_x == 0.0f);
}

static void test_five_point_independence(void)
{
    const gamepad_module_instance_t joystick = make_analog(10,
                                                           GAMEPAD_MODULE_JOYSTICK,
                                                           GAMEPAD_ACTION_MOVE,
                                                           0,
                                                           300,
                                                           200,
                                                           200);
    const gamepad_module_instance_t view = make_analog(11,
                                                       GAMEPAD_MODULE_VIEW_SLIDER,
                                                       GAMEPAD_ACTION_LOOK,
                                                       600,
                                                       250,
                                                       300,
                                                       250);
    const gamepad_module_instance_t button_a = make_button(12, GAMEPAD_ACTION_BTN_A, 250, 300);
    const gamepad_module_instance_t button_b = make_button(13, GAMEPAD_ACTION_BTN_B, 370, 300);
    const gamepad_module_instance_t button_x = make_button(14, GAMEPAD_ACTION_BTN_X, 490, 300);
    const uint16_t x1[] = {160, 650, 270, 390, 510};
    const uint16_t x2[] = {160, 658, 270, 390, 510};
    const uint16_t y[] = {400, 350, 350, 350, 350};
    const uint8_t ids[] = {1, 2, 3, 4, 5};
    const gamepad_input_state_t * state;

    gamepad_multitouch_input_reset();
    gamepad_multitouch_input_register_module(&joystick);
    gamepad_multitouch_input_register_module(&view);
    gamepad_multitouch_input_register_module(&button_a);
    gamepad_multitouch_input_register_module(&button_b);
    gamepad_multitouch_input_register_module(&button_x);

    gamepad_multitouch_input_on_points_at_time(x1, y, ids, 5, 200U);
    gamepad_multitouch_input_on_points_at_time(x2, y, ids, 5, 208U);
    state = gamepad_input_get_state();
    assert(state->move_x > 0.0f);
    assert(state->look_x > 0.0f);
    assert(state->btn_a);
    assert(state->btn_b);
    assert(state->btn_x);
}

int main(void)
{
    const gamepad_module_instance_t button_a = make_button(1, GAMEPAD_ACTION_BTN_A, 20, 20);
    const gamepad_module_instance_t button_b = make_button(2, GAMEPAD_ACTION_BTN_B, 200, 20);
    const uint16_t two_x[] = {50, 250};
    const uint16_t two_y[] = {50, 50};
    const uint16_t one_x[] = {250};
    const uint16_t one_y[] = {50};
    const uint16_t five_x[] = {50, 250, 450, 650, 850};
    const uint16_t five_y[] = {50, 50, 50, 50, 50};
    const uint8_t five_ids[] = {11, 12, 13, 14, 15};
    const gamepad_input_state_t * state;

    gamepad_input_state_reset();
    gamepad_multitouch_input_reset();
    gamepad_multitouch_input_register_module(&button_a);
    gamepad_multitouch_input_register_module(&button_b);
    for(uint32_t i = 0; i < 3; i++) {
        gamepad_module_instance_t button = make_button(3 + i,
                                                       GAMEPAD_ACTION_BTN_X + i,
                                                       400 + (int32_t)i * 200,
                                                       20);
        gamepad_multitouch_input_register_module(&button);
    }
    gamepad_multitouch_input_set_visual_callback(capture_visual_frame, NULL);
    gamepad_multitouch_input_set_enabled(true);

    gamepad_multitouch_input_on_points_xy(two_x, two_y, 2);
    state = gamepad_input_get_state();
    assert(state->btn_a);
    assert(state->btn_b);

    gamepad_multitouch_input_on_points_xy(one_x, one_y, 1);
    state = gamepad_input_get_state();
    assert(!state->btn_a);
    assert(state->btn_b);

    gamepad_multitouch_input_on_points_xy(NULL, NULL, 0);
    state = gamepad_input_get_state();
    assert(!state->btn_a);
    assert(!state->btn_b);

    gamepad_multitouch_input_on_points(five_x, five_y, five_ids, 5);
    assert(visual_point_count == 5);
    for(uint8_t i = 0; i < 5; i++) {
        assert(visual_points[i].track_id == five_ids[i]);
        assert(visual_points[i].module_id == (uint32_t)i + 1U);
    }

    test_view_track_lock_and_release();
    test_five_point_independence();

    puts("gamepad_multitouch_input_test: PASS");
    return 0;
}
