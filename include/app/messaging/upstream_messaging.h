#ifndef UPSTREAM_MESSAGING_H
#define UPSTREAM_MESSAGING_H

#include "generated/config_in_use.h"

// Upstream messaging: sending messages to the broker and receiving them from it.

// The config for this build sets how long the broker has to confirm a message. There is no fallback.
#ifndef BROKER_CONFIRM_LIMIT_MS
#error "BROKER_CONFIRM_LIMIT_MS is not set in the config: how long the bridge waits for the broker to confirm a message, in milliseconds"
#endif

/**
 * @brief Starts the upstream messaging.
 *
 * From then on: each message the broker confirms is answered to the MCU, and one not confirmed
 * within BROKER_CONFIRM_LIMIT_MS is reported to it; the topics in DOWNLINK_TOPICS are subscribed
 * to on every broker connection, and each message on them is sent straight down to the MCU.
 *
 * Needs mqtt_link_init() run first.
 */
void start_upstream_messaging(void);

/**
 * @brief Sends a message upstream, unchanged, to the topic its first letter picks
 *        (UPLINK_ROUTES in the config).
 *
 * An empty message is ignored. A message whose letter isn't in the table is ignored and logged.
 * While Wi-Fi or the broker is down the message is not sent, and the MCU is answered at once
 * with STATUS_CON_ERR_WIFI or STATUS_CON_ERR_BROKER.
 * A message that is sent is held until the broker confirms it: the MCU is then answered with
 * STATUS_OK, or with STATUS_ERR_BROKER if the broker has not confirmed it in time.
 *
 * @param message The message, ending in '\0'
 */
void send_message_upstream(const char *message);

#endif
