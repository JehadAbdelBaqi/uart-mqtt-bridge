#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "dummy_source.h"
#include "led.h"
#include "mqtt_link.h"
#include "router.h"
#include "uart_link.h"
#include "wifi_link.h"

/**
 * @brief Starts NVS, the flash store the Wi-Fi driver keeps its calibration data in.
 *
 * If NVS is full, or was written by a different ESP-IDF version, it is erased and started
 * afresh; the calibration data is rebuilt automatically.
 */
static void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/**
 * @brief Entry point, called by ESP-IDF once the system has started.
 *
 * Sets up what every module shares, then each module, and returns; the UART, Wi-Fi,
 * MQTT client and LED keep running in their own tasks.
 */
void app_main(void)
{
    // Shared by every module: started first
    nvs_init();
    ESP_ERROR_CHECK(esp_netif_init());                 // network interface layer (TCP/IP)
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // loop the Wi-Fi, IP and MQTT events arrive on

    led_init();
    led_startup();
    uart_link_init();
    wifi_link_init();
    mqtt_link_init();
    router_init();
    dummy_source_init();
}
