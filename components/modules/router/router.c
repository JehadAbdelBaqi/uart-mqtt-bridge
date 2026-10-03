#include "router.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "config.h"
#include "mqtt_link.h"
#include "uart_link.h"

static const char *TAG = "router";

// One row of the uplink table: lines starting with this letter go to this topic
typedef struct {
    char letter;
    const char *topic;
} route_t;

static const route_t routes[] = { UPLINK_ROUTES };

#define ROUTE_COUNT (sizeof(routes) / sizeof(routes[0]))

// The topics passed down to the MCU
static const char *const downlink_topics[] = { DOWNLINK_TOPICS };

#define DOWNLINK_TOPIC_COUNT (sizeof(downlink_topics) / sizeof(downlink_topics[0]))

const char *router_find_topic(char letter)
{
    for (size_t i = 0; i < ROUTE_COUNT; i++) {
        if (routes[i].letter == letter) {
            return routes[i].topic;
        }
    }

    return NULL;
}

/**
 * @brief Publishes a line to a topic at QoS 1, unchanged.
 *
 * @param topic The topic to publish to
 * @param line  The line, ending in '\0'
 */
static void publish(const char *topic, const char *line)
{
    int message_id = esp_mqtt_client_publish(mqtt_link_client(), topic, line, 0, 1, 0);  // length 0 = up to the '\0', QoS 1, not retained
    if (message_id < 0) {
        ESP_LOGW(TAG, "publish to %s failed", topic);
    }
}

void router_uplink(const char *line)
{
    if (line[0] == '\0') {
        return;  // empty line: nothing to publish
    }

    const char *topic = router_find_topic(line[0]);
    if (topic == NULL) {
        ESP_LOGW(TAG, "line ignored: no topic for letter '%c'", line[0]);
        return;
    }

    publish(topic, line);
}

/**
 * @brief Subscribes at QoS 1 to every topic in DOWNLINK_TOPICS.
 *
 * The broker forgets the subscriptions when the connection drops, so this runs on every connection.
 */
static void subscribe_to_downlink_topics(void)
{
    for (size_t i = 0; i < DOWNLINK_TOPIC_COUNT; i++) {
        int message_id = esp_mqtt_client_subscribe(mqtt_link_client(), downlink_topics[i], 1);  // QoS 1
        if (message_id < 0) {
            ESP_LOGW(TAG, "subscribe to %s failed", downlink_topics[i]);
            continue;
        }

        ESP_LOGI(TAG, "subscribed to %s", downlink_topics[i]);
    }
}

/**
 * @brief Writes a message from the broker to the MCU as one line, unchanged.
 *
 * An empty message is ignored. A message longer than LINE_MAX_LEN, or one
 * containing '\n', is dropped and logged.
 *
 * @param message The message's bytes; not ending in '\0'
 * @param length  How many bytes the whole message has
 */
static void downlink(const char *message, size_t length)
{
    if (length == 0) {
        return;  // empty message: nothing to write
    }

    if (length > LINE_MAX_LEN) {
        ESP_LOGW(TAG, "message dropped: longer than %d characters", LINE_MAX_LEN);
        return;
    }

    if (memchr(message, '\n', length) != NULL) {
        ESP_LOGW(TAG, "message dropped: it contains a line ending");
        return;
    }

    char line[LINE_MAX_LEN + 2];  // the message, then '\n' and '\0'
    memcpy(line, message, length);
    line[length] = '\n';
    line[length + 1] = '\0';

    ESP_LOGI(TAG, "down: %.*s", (int)length, message);
    uart_link_send(line);
}

/**
 * @brief Passes a received message to downlink().
 *
 * A message too big for the client's buffer arrives in several parts. Only the first
 * part is passed on, with the whole message's length, so it is dropped as too long.
 *
 * @param event The data event's details
 */
static void handle_message(esp_mqtt_event_handle_t event)
{
    if (event->current_data_offset > 0) {
        return;  // a later part of a long message, already dropped at its first part
    }

    downlink(event->data, event->total_data_len);
}

/**
 * @brief Reacts to the message events from the MQTT client: connected (subscribe), message received.
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
        subscribe_to_downlink_topics();
        break;
    case MQTT_EVENT_DATA:
        handle_message(event);
        break;
    default:
        break;  // the connection's events are mqtt_link's job
    }
}

void router_init(void)
{
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_link_client(), ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
}
