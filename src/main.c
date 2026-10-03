#include "led.h"
#include "mqtt_link.h"
#include "uart_link.h"
#include "wifi_link.h"

/**
 * @brief Entry point, called by ESP-IDF once the system has started.
 *
 * Sets everything up and returns; the UART, Wi-Fi, MQTT client and LED keep running in their own tasks.
 */
void app_main(void)
{
    led_init();
    led_startup();
    uart_link_init();
    wifi_link_init();
    mqtt_link_init();  // after wifi_link_init: it listens on the event loop that creates
}
