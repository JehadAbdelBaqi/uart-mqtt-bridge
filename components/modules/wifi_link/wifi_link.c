#include "wifi_link.h"

#include <stdbool.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "led.h"
#include "secrets/wifi.h"

#define RETRY_DELAY_S  5                            // wait after a drop before connecting again
#define RETRY_DELAY_US (RETRY_DELAY_S * 1000000LL)  // the same, as the timer takes it

static const char *TAG = "wifi";

static esp_timer_handle_t retry_timer;

// Whether Wi-Fi is connected and has an address
static bool wifi_is_up = false;

/**
 * @brief Starts a connection attempt and shows it on the LED.
 */
static void wifi_connect(void)
{
    ESP_LOGI(TAG, "connecting to '%s'...", WIFI_SSID);
    led_show_wifi(LED_WIFI_CONNECTING);
    esp_wifi_connect();
}

/**
 * @brief Called by the retry timer once the wait after a drop is over.
 *
 * @param arg Not used
 */
static void retry_timer_callback(void *arg)
{
    wifi_connect();
}

/**
 * @brief Reacts to Wi-Fi events: radio started -> connect; disconnected -> LED red,
 *        connect again after RETRY_DELAY_S.
 *
 * @param arg        Not used
 * @param event_base Always WIFI_EVENT
 * @param event_id   Which Wi-Fi event
 * @param event_data For a disconnect, the reason (wifi_event_sta_disconnected_t)
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGW(TAG, "not connected (reason %d), trying again in %d s", disconnected->reason, RETRY_DELAY_S);
        wifi_is_up = false;
        led_show_wifi(LED_WIFI_DOWN);
        esp_timer_start_once(retry_timer, RETRY_DELAY_US);  // the handler must not wait, so a timer does it
        break;
    default:
        break;  // other Wi-Fi events aren't used
    }
}

/**
 * @brief Logs the address Wi-Fi received and turns the LED green.
 *
 * @param arg        Not used
 * @param event_base Always IP_EVENT
 * @param event_id   Always IP_EVENT_STA_GOT_IP
 * @param event_data The address (ip_event_got_ip_t)
 */
static void got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;

    ESP_LOGI(TAG, "connected, address " IPSTR, IP2STR(&got_ip->ip_info.ip));
    wifi_is_up = true;
    led_show_wifi(LED_WIFI_UP);
}

void wifi_link_init(void)
{
    esp_netif_create_default_wifi_sta();  // interface that will hold the bridge's IP address

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));     // Wi-Fi driver with ESP-IDF's default settings
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  // station: join a network, don't create one

    esp_timer_create_args_t retry_timer_args = {
        .callback = retry_timer_callback,
        .name = "wifi_retry",
    };
    ESP_ERROR_CHECK(esp_timer_create(&retry_timer_args, &retry_timer));

    // Registered before the radio starts, so the "started" event isn't missed
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());  // radio on; the handler connects once it has started
}

bool check_wifi_link(void)
{
    return wifi_is_up;
}
