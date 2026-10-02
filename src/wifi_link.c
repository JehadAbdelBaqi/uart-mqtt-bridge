#include "wifi_link.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "secrets/wifi.h"

static const char *TAG = "wifi";

void wifi_link_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());                 // flash store the Wi-Fi driver keeps its calibration data in
    ESP_ERROR_CHECK(esp_netif_init());                 // network interface layer (TCP/IP)
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // loop the Wi-Fi events arrive on

    esp_netif_create_default_wifi_sta();  // interface that will hold the bridge's IP address

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));     // Wi-Fi driver with ESP-IDF's default settings
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  // station: join a network, don't create one

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());  // radio on; nothing asks it to connect yet

    ESP_LOGI(TAG, "started in station mode for network '%s'", WIFI_SSID);
}
