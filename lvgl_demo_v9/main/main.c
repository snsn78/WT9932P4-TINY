/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "lvgl.h"
#include "core/lv_refr.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "xbox_touch_gamepad_layout_demo.h"

static const char * TAG = "xbox_touch_ui";

void app_main(void)
{
    lv_display_t * disp;
    bool locked;

    ESP_LOGI(TAG, "GamePad firmware booting");

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
        }
    };
    ESP_LOGI(TAG, "Starting BSP display");
    disp = bsp_display_start_with_config(&cfg);
    if(disp == NULL) {
        ESP_LOGE(TAG, "bsp_display_start_with_config failed");
        return;
    }
    ESP_LOGI(TAG, "Display started: %p", disp);

    bsp_display_backlight_on();
    ESP_LOGI(TAG, "Backlight on");

    ESP_LOGI(TAG, "Mounting SPIFFS");
    esp_err_t spiffs_ret = bsp_spiffs_mount();
    if(spiffs_ret != ESP_OK && spiffs_ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPIFFS mount failed: %s", esp_err_to_name(spiffs_ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS ready: %s", esp_err_to_name(spiffs_ret));
    }

    ESP_LOGI(TAG, "Locking LVGL");
    locked = bsp_display_lock(0);
    if(!locked) {
        ESP_LOGE(TAG, "bsp_display_lock failed");
        return;
    }

    ESP_LOGI(TAG, "Creating GamePad UI");
    xbox_touch_gamepad_layout_demo_create();
    ESP_LOGI(TAG, "GamePad UI created");
    lv_refr_now(disp);
    ESP_LOGI(TAG, "Initial LVGL refresh requested");

    bsp_display_unlock();
    ESP_LOGI(TAG, "LVGL unlocked");
}
