#include "mqtt_link.h"

#include <stdbool.h>
#include <stdlib.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mqtt_client.h"

#include "led.h"
#include "secrets/broker.h"

static const char *TAG = "mqtt";

// The certificate files embedded at build time (see src/CMakeLists.txt), each ending in '\0'
extern const char ca_crt_start[]     asm("_binary_ca_crt_start");
extern const char client_crt_start[] asm("_binary_client_crt_start");
extern const char client_key_start[] asm("_binary_client_key_start");

static esp_mqtt_client_handle_t client;
static bool client_started = false;

// Whether the client is connected to the broker
static bool mqtt_is_up = false;

// The functions to call when the connection is made and when a message arrives
static mqtt_link_handlers_t handlers = { NULL, NULL, NULL };

/**
 * @brief Tells whoever asked that the connection to the broker has been made.
 */
static void tell_connected(void)
{
    if (handlers.connected == NULL) {
        return;
    }
    handlers.connected();
}

/**
 * @brief Hands a received message to whoever asked for them.
 *
 * A message too big for the client's buffer arrives in several parts. Only the first part is
 * handed over, with the whole message's length.
 *
 * @param event The data event's details
 */
static void tell_message(esp_mqtt_event_handle_t event)
{
    if (handlers.message == NULL) {
        return;
    }
    if (event->current_data_offset > 0) {
        return;  // a later part of a long message: its first part has been handed over
    }

    handlers.message(event->data, event->total_data_len);
}

/**
 * @brief Logs what went wrong in an MQTT error event.
 *
 * @param error The error's details from the event
 */
static void log_mqtt_error(const esp_mqtt_error_codes_t *error)
{
    switch (error->error_type) {
    case MQTT_ERROR_TYPE_TCP_TRANSPORT:
        ESP_LOGE(TAG, "connection error: TLS error 0x%x, socket errno %d",
                 error->esp_tls_last_esp_err, error->esp_transport_sock_errno);
        break;
    case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
        ESP_LOGE(TAG, "broker refused the connection, code %d", error->connect_return_code);
        break;
    default:
        ESP_LOGE(TAG, "error, type %d", error->error_type);
        break;
    }
}

/**
 * @brief Tells whoever asked that the broker has confirmed a published message.
 *
 * @param message_id The ID mqtt_link_publish() gave back for that message
 */
static void tell_published(int message_id)
{
    if (handlers.published == NULL) {
        return;
    }
    handlers.published(message_id);
}

/**
 * @brief Reacts to the events from the MQTT client: connected, disconnected, error, message
 *        received, message confirmed.
 *
 * Connected and disconnected are also passed to the LED (solid or blinking green).
 *
 * @param arg        Not used
 * @param event_base Always MQTT_EVENTS
 * @param event_id   Which event
 * @param event_data The event's details (esp_mqtt_event_handle_t)
 */
static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to broker");
        mqtt_is_up = true;
        led_show_broker(true);
        tell_connected();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected from broker, the client will try again");
        mqtt_is_up = false;
        led_show_broker(false);
        break;
    case MQTT_EVENT_ERROR:
        log_mqtt_error(event->error_handle);
        break;
    case MQTT_EVENT_DATA:
        tell_message(event);
        break;
    case MQTT_EVENT_PUBLISHED:
        tell_published(event->msg_id);
        break;
    default:
        break;  // other events aren't used
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
    if (client_started) {
        return;
    }

    ESP_LOGI(TAG, "connecting to broker %s:%d...", BROKER_ADDRESS, BROKER_PORT);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    client_started = true;
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
    if (client == NULL) {
        ESP_LOGE(TAG, "could not create the MQTT client");
        abort();  // stop, as ESP_ERROR_CHECK does for the other set-up steps
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_handler, NULL, NULL));
}

void mqtt_link_set_handlers(const mqtt_link_handlers_t *new_handlers)
{
    handlers = *new_handlers;
}

int mqtt_link_publish(const char *topic, const char *text)
{
    return esp_mqtt_client_publish(client, topic, text, 0, 1, 0);  // length 0 = up to the '\0', QoS 1, not retained
}

void mqtt_link_drop_connection(void)
{
    ESP_LOGW(TAG, "dropping the connection to the broker, the client will make it again");
    esp_mqtt_client_disconnect(client);
}

bool mqtt_link_subscribe(const char *topic)
{
    return esp_mqtt_client_subscribe(client, topic, 1) >= 0;  // QoS 1
}

bool check_mqtt_link(void)
{
    return mqtt_is_up;
}
