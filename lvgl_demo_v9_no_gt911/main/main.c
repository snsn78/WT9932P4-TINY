/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "lvgl.h"
#include "core/lv_refr.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

static const char *TAG = "lcd_no_gt911";

static void create_lcd_test_screen(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    static const uint32_t colors[] = {
        0xEF4444,
        0xF59E0B,
        0x22C55E,
        0x38BDF8,
        0x6366F1,
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *band = lv_obj_create(scr);
        lv_obj_set_size(band, BSP_LCD_H_RES / 5, BSP_LCD_V_RES);
        lv_obj_align(band, LV_ALIGN_LEFT_MID, i * (BSP_LCD_H_RES / 5), 0);
        lv_obj_set_style_bg_color(band, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(band, 0, 0);
        lv_obj_set_style_radius(band, 0, 0);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_size(panel, 560, 190);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_90, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(panel, LV_OPA_40, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 6, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "WT9932P4 LCD OK");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *subtitle = lv_label_create(panel);
    lv_label_set_text(subtitle, "Display-only test: GT911 touch is not initialized");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_18, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *footer = lv_label_create(panel);
    lv_label_set_text(footer, "1024 x 600 MIPI-DSI");
    lv_obj_set_style_text_color(footer, lv_color_hex(0xA7F3D0), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_16, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -26);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting display-only firmware");

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

    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "bsp_display_start_with_config failed");
        return;
    }

    bsp_display_backlight_on();
    ESP_LOGI(TAG, "Backlight on");

    if (!bsp_display_lock(0)) {
        ESP_LOGE(TAG, "bsp_display_lock failed");
        return;
    }

    create_lcd_test_screen();
    lv_refr_now(disp);
    bsp_display_unlock();

    ESP_LOGI(TAG, "Display-only test screen created");
}
