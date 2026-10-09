#include "upstream_messaging.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "downstream_messaging.h"
#include "generated/config_in_use.h"
#include "helpers.h"
#include "mqtt_link.h"
#include "status.h"
#include "uart_link.h"

static const char *TAG = "upstream";

// One row of the uplink table: messages starting with this letter go to this topic
typedef struct {
    char letter;
    const char *topic;
} route_t;

static const route_t routes[] = { UPLINK_ROUTES };

#define ROUTE_COUNT (sizeof(routes) / sizeof(routes[0]))

// The topics whose messages are passed down to the MCU, from DOWNLINK_TOPICS in the config
static const char *const downlink_topics[] = { DOWNLINK_TOPICS };

#define DOWNLINK_TOPIC_COUNT (sizeof(downlink_topics) / sizeof(downlink_topics[0]))

/**
 * @brief Finds the topic for a letter in the uplink table.
 *
 * @param letter The first letter of a message
 * @return The topic, or NULL if no row has that letter
 */
static const char *find_topic(char letter)
{
    for (size_t i = 0; i < ROUTE_COUNT; i++) {
        if (routes[i].letter == letter) {
            return routes[i].topic;
        }
    }

    return NULL;
}

/** The one message that has been sent upstream and that the broker has not confirmed yet. */
typedef struct {
    bool waiting;                    // true while a message is waiting to be confirmed
    int message_id;                  // the ID the MQTT link gave it
    char message[LINE_MAX_LEN + 1];  // the message itself, ending in '\0'
} sent_message_t;

static sent_message_t sent_message = { .waiting = false };

// Runs out when the broker has not confirmed the held message within BROKER_CONFIRM_LIMIT_MS
static esp_timer_handle_t confirm_timer;

#define CONFIRM_LIMIT_US (BROKER_CONFIRM_LIMIT_MS * 1000LL)  // the limit, as the timer takes it

/**
 * @brief Holds a message that has just been sent upstream, until the broker confirms it, and
 *        starts the timer for the broker's confirmation.
 *
 * Only one message is held: holding another replaces it, and the timer starts again.
 *
 * @param message_id The ID the MQTT link gave the message
 * @param message    The message, ending in '\0'
 */
static void hold_sent_message(int message_id, const char *message)
{
    sent_message.message_id = message_id;
    strlcpy(sent_message.message, message, sizeof(sent_message.message));
    sent_message.waiting = true;

    esp_timer_stop(confirm_timer);  // in case it is still running for a message this one replaces
    esp_timer_start_once(confirm_timer, CONFIRM_LIMIT_US);
}

void send_message_upstream(const char *message)
{
    if (message[0] == '\0') {
        return;  // empty message: nothing to send
    }

    const char *topic = find_topic(message[0]);
    if (topic == NULL) {
        // TODO: handle this better than a log: the sender is not told
        ESP_LOGW(TAG, "message ignored: no topic for letter '%c'", message[0]);
        return;
    }
    if (!STATUS_IS_OK) {
        send_connection_status_downstream(message);  // not sent: the MCU is told what is down
        return;
    }

    int message_id = mqtt_link_publish(topic, message);
    if (message_id < 0) {
        // TODO: handle this better than a log: the sender is not told
        ESP_LOGW(TAG, "publish to %s failed", topic);
        return;
    }

    hold_sent_message(message_id, message);
}

/**
 * @brief Called by the MQTT link when the broker confirms a message: if it is the message being
 *        held, the MCU is answered that it has gone through, and it is held no longer.
 *
 * @param message_id The ID the MQTT link gave the message when it was sent
 */
static void handle_broker_confirmation(int message_id)
{
    if (!sent_message.waiting) {
        return;  // nothing is waiting to be confirmed
    }
    if (sent_message.message_id != message_id) {
        return;  // not the message being held
    }

    esp_timer_stop(confirm_timer);
    sent_message.waiting = false;
    send_answer_downstream(sent_message.message, STATUS_OK);
}

/**
 * @brief Called by the timer when the broker has not confirmed the held message in time: the MCU
 *        is answered that the broker has a fault, the message is held no longer, and the
 *        connection to the broker is dropped so that it is made afresh.
 *
 * @param arg Not used
 */
static void handle_confirmation_timeout(void *arg)
{
    if (!sent_message.waiting) {
        return;  // confirmed just as the timer ran out
    }

    sent_message.waiting = false;
    send_answer_downstream(sent_message.message, STATUS_ERR_BROKER);
    mqtt_link_drop_connection();
}

/**
 * @brief Called by the MQTT link each time the connection to the broker is made: subscribes to
 *        every topic in DOWNLINK_TOPICS.
 *
 * The broker forgets the subscriptions when the connection drops, so this runs on every connection.
 */
static void subscribe_to_downlink_topics(void)
{
    for (size_t i = 0; i < DOWNLINK_TOPIC_COUNT; i++) {
        if (!mqtt_link_subscribe(downlink_topics[i])) {
            // TODO: handle this better than a log
            ESP_LOGW(TAG, "subscribe to %s failed", downlink_topics[i]);
        }
    }
}

/**
 * @brief Called by the MQTT link for each message from the broker: sends it straight down to the
 *        MCU, as it is, with the line end added.
 *
 * A message the bridge cannot pass on, an empty one or one longer than the line limit, is not
 * sent: the broker can be sent anything, and a longer one would not fit the buffer.
 *
 * @param message The message's bytes; not ending in '\0'
 * @param length  How many bytes the whole message has
 */
static void pass_message_downstream(const char *message, size_t length)
{
    char line[DOWNSTREAM_LINE_SIZE];

    if (!message_is_valid(length)) {
        return;
    }

    format_message_for_downstream(message, length, line);
    uart_link_send(line);
    log_message("down", line);
}

void start_upstream_messaging(void)
{
    static const mqtt_link_handlers_t handlers = {
        .connected = subscribe_to_downlink_topics,
        .message   = pass_message_downstream,
        .published = handle_broker_confirmation,
    };
    esp_timer_create_args_t confirm_timer_args = {
        .callback = handle_confirmation_timeout,
        .name = "broker_confirm",
    };

    ESP_ERROR_CHECK(esp_timer_create(&confirm_timer_args, &confirm_timer));
    mqtt_link_set_handlers(&handlers);
}
