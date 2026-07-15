#include "gamepad_hid_device.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "class/hid/hid_device.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

#include "gamepad_hid_report.h"
#include "gamepad_hid_selftest.h"
#include "gamepad_output.h"
#include "gamepad_usb_descriptors.h"

static const char *TAG = "gamepad_hid_device";

static TaskHandle_t s_hid_task_handle = NULL;
static volatile bool s_hid_task_running = false;
static bool s_usb_initialized = false;

static esp_err_t gamepad_hid_device_usb_init(void)
{
    if(s_usb_initialized) return ESP_OK;

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = gamepad_usb_configuration_descriptor;
    tusb_cfg.descriptor.string = gamepad_usb_string_descriptor;
    tusb_cfg.descriptor.string_count = gamepad_usb_string_descriptor_count;
#if TUD_OPT_HIGH_SPEED
    tusb_cfg.descriptor.high_speed_config = gamepad_usb_configuration_descriptor;
#endif

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    s_usb_initialized = true;
    ESP_LOGI(TAG, "TinyUSB HID gamepad initialized");
    return ESP_OK;
}

static bool gamepad_hid_device_send(const gamepad_hid_report_t *hid)
{
    if(hid == NULL || !tud_mounted() || !tud_hid_ready()) return false;
    return tud_hid_report(0, hid, sizeof(*hid));
}

static void gamepad_hid_task(void *arg)
{
    gamepad_output_report_t out;
    gamepad_hid_report_t hid;
    uint32_t last_seq = UINT32_MAX;

    (void)arg;
    ESP_LOGI(TAG, "USB HID gamepad task started");

    while(s_hid_task_running) {
        if(gamepad_output_poll(&out) && out.seq != last_seq) {
            gamepad_hid_report_build(&out, &hid);
            if(gamepad_hid_device_send(&hid)) {
                last_seq = out.seq;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGI(TAG, "USB HID gamepad task stopped");
    s_hid_task_handle = NULL;
    vTaskDelete(NULL);
}

void gamepad_hid_device_start(void)
{
    if(s_hid_task_handle != NULL) {
        ESP_LOGW(TAG, "USB HID gamepad task already running");
        return;
    }

    if(gamepad_hid_device_usb_init() != ESP_OK) return;

    s_hid_task_running = true;
    BaseType_t ret = xTaskCreate(gamepad_hid_task,
                                 "gamepad_hid",
                                 4096,
                                 NULL,
                                 5,
                                 &s_hid_task_handle);
    if(ret != pdPASS) {
        s_hid_task_running = false;
        s_hid_task_handle = NULL;
        ESP_LOGE(TAG, "Failed to create USB HID gamepad task");
        return;
    }

#if defined(GAMEPAD_HID_SELFTEST)
    gamepad_hid_selftest_start();
#endif
}

void gamepad_hid_device_stop(void)
{
#if defined(GAMEPAD_HID_SELFTEST)
    gamepad_hid_selftest_stop();
#endif
    if(s_hid_task_handle == NULL) {
        ESP_LOGW(TAG, "USB HID gamepad task is not running");
        return;
    }
    s_hid_task_running = false;
}
