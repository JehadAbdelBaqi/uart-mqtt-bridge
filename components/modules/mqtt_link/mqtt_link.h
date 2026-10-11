#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include <stdbool.h>
#include <stddef.h>

/** The functions the MQTT link calls when something happens. Any of them may be NULL. */
typedef struct {
    // Called each time the connection to the broker is made. The broker forgets subscriptions when
    // the connection drops, so this is the place to subscribe.
    void (*connected)(void);

    // Called for each message received on a subscribed topic. The message does not end in '\0'.
    // length is the whole message's length: a message too big for the client's buffer arrives in
    // parts, and only its first part is given, so fewer bytes than length are there.
    void (*message)(const char *message, size_t length);

    // Called when the broker confirms a message published with mqtt_link_publish(), with the ID
    // that call gave back.
    void (*published)(int message_id);
} mqtt_link_handlers_t;

/**
 * @brief Sets up the MQTT client for the broker in secrets/broker.h.
 *
 * Connects over TLS with the embedded certificates once Wi-Fi has an address.
 * Reconnecting after a drop happens in the background.
 * Needs NVS, the network interface layer and the default event loop started first (app_main does this).
 */
void mqtt_link_init(void);

/**
 * @brief Sets the functions to call when the connection is made, when a message arrives and when
 *        a published message is confirmed.
 *
 * @param new_handlers The functions; a copy is kept
 */
void mqtt_link_set_handlers(const mqtt_link_handlers_t *new_handlers);

/**
 * @brief Publishes text to a topic at QoS 1, unchanged and not retained.
 *
 * @param topic The topic to publish to
 * @param text  The text, ending in '\0'
 * @return The ID the client gave the message, or a negative number if it could not be published
 */
int mqtt_link_publish(const char *topic, const char *text);

/**
 * @brief Drops the connection to the broker. The client makes it again by itself, after its wait.
 */
void mqtt_link_drop_connection(void);

/**
 * @brief Subscribes to a topic at QoS 1.
 *
 * @param topic The topic
 * @return true if the request was sent to the broker
 */
bool mqtt_link_subscribe(const char *topic);

/**
 * @brief Says whether the client is connected to the broker.
 *
 * @return true from the moment the broker accepts the connection until it drops
 */
bool check_mqtt_link(void);

#endif
