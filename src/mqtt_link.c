#include "mqtt_link.h"

#include <stdbool.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mqtt_client.h"

#include "secrets/broker.h"

static const char *TAG = "mqtt";

// The certificate files embedded at build time (see src/CMakeLists.txt), each ending in '\0'
extern const char ca_crt_start[]     asm("_binary_ca_crt_start");
extern const char client_crt_start[] asm("_binary_client_crt_start");
extern const char client_key_start[] asm("_binary_client_key_start");

static esp_mqtt_client_handle_t client;
static bool client_started = false;

/**
 * @brief Reacts to events from the MQTT client: connected, disconnected, error.
 *
 * @param arg        Not used
 * @param event_base Always MQTT_EVENTS
 * @param event_id   Which event
 * @param event_data The event's details (esp_mqtt_event_handle_t)
 */
static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "connected to broker %s:%d", BROKER_ADDRESS, BROKER_PORT);
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "disconnected from broker, the client will try again");
    } else if (event_id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "error, type %d", event->error_handle->error_type);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "TLS error 0x%x, socket errno %d",
                     event->error_handle->esp_tls_last_esp_err,
                     event->error_handle->esp_transport_sock_errno);
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGE(TAG, "broker refused the connection, code %d", event->error_handle->connect_return_code);
        }
    }
}

/**
 * @brief Starts the MQTT client the first time Wi-Fi gets an address.
 *
 * After that the client reconnects by itself, so later addresses are ignored.
 *
 * @param arg        Not used
 * @param event_base Always IP_EVENT
 * @param event_id   Always IP_EVENT_STA_GOT_IP
 * @param event_data Not used
 */
static void got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (!client_started) {
        ESP_LOGI(TAG, "connecting to broker %s:%d...", BROKER_ADDRESS, BROKER_PORT);
        ESP_ERROR_CHECK(esp_mqtt_client_start(client));
        client_started = true;
    }
}

void mqtt_link_init(void)
{
    esp_mqtt_client_config_t config = {
        .broker = {
            .address = {
                .hostname  = BROKER_ADDRESS,
                .port      = BROKER_PORT,
                .transport = MQTT_TRANSPORT_OVER_SSL,  // MQTT over TLS
            },
            .verification.certificate = ca_crt_start,  // checks the broker's certificate
        },
        .credentials.authentication = {
            .certificate = client_crt_start,  // the bridge's certificate
            .key         = client_key_start,  // and its private key
        },
    };

    client = esp_mqtt_client_init(&config);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_handler, NULL, NULL));
}
