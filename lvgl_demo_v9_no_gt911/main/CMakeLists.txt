idf_component_register(
    SRCS main.c
    INCLUDE_DIRS .
    PRIV_REQUIRES espressif__esp32_p4_function_ev_board)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
)
