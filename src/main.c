#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "uart_link.h"
#include "wifi_link.h"

/**
 * @brief Entry point, called by ESP-IDF once the system has started.
 */
void app_main(void)
{
    led_init();
    led_startup();
    uart_link_init();
    wifi_link_init();

    while (1) {
        uart_link_send("hello\n");  // loopback test: GPIO7 wired to GPIO6
        vTaskDelay(pdMS_TO_TICKS(1000));  // wait 1 s
    }
}
