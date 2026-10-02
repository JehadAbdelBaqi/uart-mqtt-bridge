#include "wifi_link.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";

void wifi_link_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());                 // flash store the Wi-Fi driver keeps its calibration data in
    ESP_ERROR_CHECK(esp_netif_init());                 // network interface layer (TCP/IP)
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // loop the Wi-Fi events arrive on

    ESP_LOGI(TAG, "groundwork ready");
}
