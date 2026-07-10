set(GAMEPAD_UI_SOURCES
    ../../src/gamepad_layout_model.c
    ../../src/gamepad_input_state.c
    ../../src/gamepad_action_mapper.c
    ../../src/gamepad_output.c
    ../../src/gamepad_hid_report.c
    ../../src/gamepad_debug_overlay.c
    ../../src/gamepad_button_module.c
    ../../src/joystick_stick_demo.c
    ../../src/Dpad.c
    ../../src/shoulder_keys_demo.c
    ../../src/codex_gpt5_slider_2d_demo.c
    ../../src/xbox_touch_gamepad_layout_demo.c
)

idf_component_register(
    SRCS main.c ${GAMEPAD_UI_SOURCES}
    INCLUDE_DIRS . ../../src
    PRIV_REQUIRES espressif__esp32_p4_function_ev_board nvs_flash bsp_extra)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
)
