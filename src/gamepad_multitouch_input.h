#ifndef GAMEPAD_MULTITOUCH_INPUT_H
#define GAMEPAD_MULTITOUCH_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "gamepad_layout_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAMEPAD_MULTITOUCH_MAX_POINTS 5U

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t track_id;
    uint32_t module_id;
} gamepad_multitouch_visual_point_t;

typedef void (*gamepad_multitouch_visual_cb_t)(const gamepad_multitouch_visual_point_t * points,
                                               uint8_t point_count,
                                               void * user_data);

/*
 * GT911/LVGL multi-touch bridge for gamepad PLAY mode.
 *
 * The LVGL port still owns the actual GT911 read. It calls the weak hook
 * gamepad_multitouch_input_on_points_xy() after reading all touch points. This
 * module then hit-tests every active point against registered layout modules
 * and writes simultaneous states into gamepad_input_state.
 */

void gamepad_multitouch_input_reset(void);
void gamepad_multitouch_input_register_module(const gamepad_module_instance_t * module);
void gamepad_multitouch_input_set_enabled(bool enabled);
bool gamepad_multitouch_input_is_enabled(void);
void gamepad_multitouch_input_set_visual_callback(gamepad_multitouch_visual_cb_t callback,
                                                  void * user_data);

void gamepad_multitouch_input_on_points(const uint16_t * x,
                                        const uint16_t * y,
                                        const uint8_t * track_id,
                                        uint8_t point_count);
void gamepad_multitouch_input_on_points_at_time(const uint16_t * x,
                                                const uint16_t * y,
                                                const uint8_t * track_id,
                                                uint8_t point_count,
                                                uint32_t now_ms);
void gamepad_multitouch_input_on_points_xy(const uint16_t * x, const uint16_t * y, uint8_t point_count);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_MULTITOUCH_INPUT_H */
