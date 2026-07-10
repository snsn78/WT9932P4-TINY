#ifndef GAMEPAD_MODULE_BUILDERS_H
#define GAMEPAD_MODULE_BUILDERS_H

#include "lvgl.h"

lv_obj_t * joystick_stick_create_compact(lv_obj_t * parent);
lv_obj_t * joystick_stick_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height);

lv_obj_t * dpad_create_compact(lv_obj_t * parent);
lv_obj_t * dpad_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height);

lv_obj_t * shoulder_keys_create_left_compact(lv_obj_t * parent);
lv_obj_t * shoulder_keys_create_left_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height);
lv_obj_t * shoulder_keys_create_right_compact(lv_obj_t * parent);
lv_obj_t * shoulder_keys_create_right_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height);
lv_obj_t * shoulder_keys_create_compact(lv_obj_t * parent);

lv_obj_t * slider_2d_create_compact(lv_obj_t * parent);
lv_obj_t * slider_2d_create_compact_sized(lv_obj_t * parent, lv_coord_t width, lv_coord_t height);

lv_obj_t * gamepad_button_create_compact(lv_obj_t * parent, const char * text);
lv_obj_t * gamepad_button_create_compact_sized(lv_obj_t * parent,
                                               const char * text,
                                               lv_coord_t width,
                                               lv_coord_t height);

#endif
