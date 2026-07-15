#include "gamepad_view_motion.h"

#include <math.h>
#include <string.h>

static float clampf_local(float value, float lo, float hi)
{
    if(value < lo) return lo;
    if(value > hi) return hi;
    return value;
}

static float curve_component(float value, float exponent)
{
    if(value == 0.0f) return 0.0f;
    return copysignf(powf(fabsf(value), exponent), value);
}

gamepad_view_motion_config_t gamepad_view_motion_default_config(void)
{
    const gamepad_view_motion_config_t config = {
        .pixels_per_ms_for_full_scale = 0.80f,
        .deadzone_velocity = 0.010f,
        .exponent = 1.0f,
        .max_output = 1.0f,
        .filter_tau_seconds = 0.010f,
    };
    return config;
}

void gamepad_view_motion_reset(gamepad_view_motion_state_t * state)
{
    if(state != NULL) memset(state, 0, sizeof(*state));
}

gamepad_view_motion_output_t gamepad_view_motion_calculate_target(
    const gamepad_view_motion_config_t * config,
    int32_t dx,
    int32_t dy,
    uint32_t dt_ms)
{
    gamepad_view_motion_output_t target = {0.0f, 0.0f};
    float raw_x;
    float raw_y;
    float magnitude;

    if(config == NULL || config->pixels_per_ms_for_full_scale <= 0.0f) return target;
    if(dt_ms < 4U) dt_ms = 4U;
    if(dt_ms > 40U) dt_ms = 40U;

    raw_x = ((float)dx / (float)dt_ms) / config->pixels_per_ms_for_full_scale;
    raw_y = ((float)dy / (float)dt_ms) / config->pixels_per_ms_for_full_scale;
    raw_x = clampf_local(raw_x, -1.0f, 1.0f);
    raw_y = clampf_local(raw_y, -1.0f, 1.0f);
    magnitude = sqrtf(raw_x * raw_x + raw_y * raw_y);

    if(magnitude > config->deadzone_velocity && magnitude > 1e-6f) {
        const float scaled_magnitude = clampf_local(
            (magnitude - config->deadzone_velocity) / (1.0f - config->deadzone_velocity),
            0.0f,
            1.0f);
        const float radial_scale = scaled_magnitude / magnitude;
        target.x = curve_component(raw_x * radial_scale, config->exponent) * config->max_output;
        target.y = curve_component(raw_y * radial_scale, config->exponent) * config->max_output;
    }
    return target;
}

gamepad_view_motion_output_t gamepad_view_motion_begin(gamepad_view_motion_state_t * state,
                                                       int32_t x,
                                                       int32_t y,
                                                       uint32_t now_ms)
{
    const gamepad_view_motion_output_t zero = {0.0f, 0.0f};
    if(state == NULL) return zero;

    gamepad_view_motion_reset(state);
    state->active = true;
    state->last_x = x;
    state->last_y = y;
    state->last_time_ms = now_ms;
    return zero;
}

gamepad_view_motion_output_t gamepad_view_motion_update(gamepad_view_motion_state_t * state,
                                                        const gamepad_view_motion_config_t * config,
                                                        int32_t x,
                                                        int32_t y,
                                                        uint32_t now_ms)
{
    gamepad_view_motion_output_t output = {0.0f, 0.0f};
    uint32_t dt_ms;
    gamepad_view_motion_output_t target;
    float alpha;

    if(state == NULL || config == NULL || !state->active) return output;

    dt_ms = now_ms - state->last_time_ms;
    if(dt_ms < 4U) dt_ms = 4U;
    if(dt_ms > 40U) dt_ms = 40U;

    target = gamepad_view_motion_calculate_target(config,
                                                  x - state->last_x,
                                                  state->last_y - y,
                                                  dt_ms);

    alpha = 1.0f - expf(-((float)dt_ms / 1000.0f) / config->filter_tau_seconds);
    state->filtered_x += alpha * (target.x - state->filtered_x);
    state->filtered_y += alpha * (target.y - state->filtered_y);
    state->last_x = x;
    state->last_y = y;
    state->last_time_ms = now_ms;

    output.x = clampf_local(state->filtered_x, -config->max_output, config->max_output);
    output.y = clampf_local(state->filtered_y, -config->max_output, config->max_output);
    return output;
}

gamepad_view_motion_output_t gamepad_view_motion_release(gamepad_view_motion_state_t * state)
{
    const gamepad_view_motion_output_t zero = {0.0f, 0.0f};
    gamepad_view_motion_reset(state);
    return zero;
}
