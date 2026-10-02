#include "wifi_link.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "secrets/wifi.h"

static const char *TAG = "wifi";

/**
 * @brief Reacts to Wi-Fi and IP events from the default event loop.
 *
 * Radio started -> connect. Disconnected -> connect again.
 * Address received -> log it.
 *
 * @param arg        Not used
 * @param event_base Which group the event belongs to: WIFI_EVENT or IP_EVENT
 * @param event_id   Which event in that group
 * @param event_data Details of the event; for IP_EVENT_STA_GOT_IP, the address
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "connecting to '%s'...", WIFI_SSID);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected, connecting again...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "connected, address " IPSTR, IP2STR(&got_ip->ip_info.ip));
    }
}

void wifi_link_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());                 // flash store the Wi-Fi driver keeps its calibration data in
    ESP_ERROR_CHECK(esp_netif_init());                 // network interface layer (TCP/IP)
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // loop the Wi-Fi events arrive on

    esp_netif_create_default_wifi_sta();  // interface that will hold the bridge's IP address

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));     // Wi-Fi driver with ESP-IDF's default settings
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  // station: join a network, don't create one

    // Registered before the radio starts, so the "started" event isn't missed
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());  // radio on; the handler connects once it has started
}
