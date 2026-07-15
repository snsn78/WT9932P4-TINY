#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "gamepad_view_motion.h"

static bool nearly_equal(float a, float b, float tolerance)
{
    return fabsf(a - b) <= tolerance;
}

int main(void)
{
    const gamepad_view_motion_config_t config = gamepad_view_motion_default_config();
    gamepad_view_motion_state_t state;
    gamepad_view_motion_output_t output;
    gamepad_view_motion_output_t short_interval_target;
    gamepad_view_motion_output_t long_interval_target;

    assert(nearly_equal(config.pixels_per_ms_for_full_scale, 0.80f, 0.001f));
    assert(nearly_equal(config.exponent, 1.0f, 0.001f));
    assert(nearly_equal(config.max_output, 1.0f, 0.001f));
    assert(nearly_equal(config.filter_tau_seconds, 0.010f, 0.001f));

    gamepad_view_motion_reset(&state);
    output = gamepad_view_motion_begin(&state, 400, 300, 100U);
    assert(output.x == 0.0f);
    assert(output.y == 0.0f);

    output = gamepad_view_motion_update(&state, &config, 408, 300, 108U);
    assert(output.x > 0.0f);
    assert(output.y == 0.0f);
    short_interval_target = gamepad_view_motion_calculate_target(&config, 8, 0, 8U);
    long_interval_target = gamepad_view_motion_calculate_target(&config, 16, 0, 16U);
    assert(nearly_equal(short_interval_target.x, long_interval_target.x, 0.001f));

    gamepad_view_motion_reset(&state);
    (void)gamepad_view_motion_begin(&state, 400, 300, 100U);
    output = gamepad_view_motion_update(&state, &config, 400, 292, 108U);
    assert(output.y > 0.0f);

    gamepad_view_motion_reset(&state);
    (void)gamepad_view_motion_begin(&state, 400, 300, 100U);
    output = gamepad_view_motion_update(&state, &config, 400, 300, 108U);
    assert(output.x == 0.0f);
    assert(output.y == 0.0f);

    gamepad_view_motion_reset(&state);
    (void)gamepad_view_motion_begin(&state, 400, 300, 100U);
    output = gamepad_view_motion_update(&state, &config, 1000, 300, 108U);
    assert(output.x <= config.max_output);
    assert(output.x > 0.0f);

    output = gamepad_view_motion_update(&state, &config, 1000, 300, 148U);
    assert(output.x < 0.10f);

    output = gamepad_view_motion_release(&state);
    assert(output.x == 0.0f);
    assert(output.y == 0.0f);
    assert(!state.active);

    puts("gamepad_view_motion_test: PASS");
    return 0;
}
