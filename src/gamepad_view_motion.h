#ifndef GAMEPAD_VIEW_MOTION_H
#define GAMEPAD_VIEW_MOTION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float pixels_per_ms_for_full_scale;
    float deadzone_velocity;
    float exponent;
    float max_output;
    float filter_tau_seconds;
} gamepad_view_motion_config_t;

typedef struct {
    bool active;
    int32_t last_x;
    int32_t last_y;
    uint32_t last_time_ms;
    float filtered_x;
    float filtered_y;
} gamepad_view_motion_state_t;

typedef struct {
    float x;
    float y;
} gamepad_view_motion_output_t;

gamepad_view_motion_config_t gamepad_view_motion_default_config(void);
void gamepad_view_motion_reset(gamepad_view_motion_state_t * state);
gamepad_view_motion_output_t gamepad_view_motion_calculate_target(
    const gamepad_view_motion_config_t * config,
    int32_t dx,
    int32_t dy,
    uint32_t dt_ms);
gamepad_view_motion_output_t gamepad_view_motion_begin(gamepad_view_motion_state_t * state,
                                                       int32_t x,
                                                       int32_t y,
                                                       uint32_t now_ms);
gamepad_view_motion_output_t gamepad_view_motion_update(gamepad_view_motion_state_t * state,
                                                        const gamepad_view_motion_config_t * config,
                                                        int32_t x,
                                                        int32_t y,
                                                        uint32_t now_ms);
gamepad_view_motion_output_t gamepad_view_motion_release(gamepad_view_motion_state_t * state);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_VIEW_MOTION_H */
