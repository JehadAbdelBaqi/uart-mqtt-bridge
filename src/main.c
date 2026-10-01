#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"

static const char *TAG = "bridge";

static void second_task(void *arg)
{
    while (1) {
        ESP_LOGI(TAG, "second task");
        vTaskDelay(pdMS_TO_TICKS(3000));  // wait 3 s
    }
}

void app_main(void)
{
    led_init();
    led_startup();
    xTaskCreate(second_task, "second", 4096, NULL, 5, NULL);

    while (1) {
        ESP_LOGI(TAG, "uart-mqtt-bridge");
        vTaskDelay(pdMS_TO_TICKS(1000));  // wait 1 s
    }
}
