#include "gamepad_hid_selftest.h"

#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gamepad_input_state.h"

static const char *TAG = "gamepad_hid_test";
static TaskHandle_t s_selftest_task = NULL;
static volatile bool s_selftest_running = false;

static bool selftest_delay(TickType_t ticks)
{
    vTaskDelay(ticks);
    return s_selftest_running;
}

static void gamepad_hid_selftest_task(void *arg)
{
    const TickType_t step_delay = pdMS_TO_TICKS(500);
    (void)arg;

    ESP_LOGW(TAG, "USB HID self-test enabled; synthetic input is active");
    while(s_selftest_running) {
        gamepad_input_set_button(GAMEPAD_BTN_A, true);
        if(!selftest_delay(step_delay)) break;
        gamepad_input_set_button(GAMEPAD_BTN_A, false);

        gamepad_input_set_move(1.0f, 0.0f);
        if(!selftest_delay(step_delay)) break;
        gamepad_input_set_move(0.0f, 0.0f);

        gamepad_input_set_dpad(GAMEPAD_DPAD_UP, true);
        if(!selftest_delay(step_delay)) break;
        gamepad_input_set_dpad(GAMEPAD_DPAD_UP, false);

        gamepad_input_set_look(1.0f, -1.0f);
        if(!selftest_delay(step_delay)) break;
        gamepad_input_set_look(0.0f, 0.0f);
        if(!selftest_delay(step_delay)) break;
    }

    gamepad_input_set_button(GAMEPAD_BTN_A, false);
    gamepad_input_set_move(0.0f, 0.0f);
    gamepad_input_set_dpad(GAMEPAD_DPAD_UP, false);
    gamepad_input_set_look(0.0f, 0.0f);
    s_selftest_task = NULL;
    vTaskDelete(NULL);
}

void gamepad_hid_selftest_start(void)
{
    if(s_selftest_task != NULL) return;

    s_selftest_running = true;
    BaseType_t ret = xTaskCreate(gamepad_hid_selftest_task,
                                 "gamepad_hid_test",
                                 3072,
                                 NULL,
                                 4,
                                 &s_selftest_task);
    if(ret != pdPASS) {
        s_selftest_running = false;
        s_selftest_task = NULL;
        ESP_LOGE(TAG, "Failed to create USB HID self-test task");
    }
}

void gamepad_hid_selftest_stop(void)
{
    s_selftest_running = false;
}
