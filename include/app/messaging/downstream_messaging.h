#ifndef DOWNSTREAM_MESSAGING_H
#define DOWNSTREAM_MESSAGING_H

#include <stdbool.h>
#include <stddef.h>

#include "generated/config_in_use.h"
#include "helpers.h"
#include "status.h"

// Downstream messaging: sending messages to the MCU and receiving them from it.

// The config for this build says what the bridge's answer looks like. There is no fallback.
#ifndef ANSWER_FORMAT
#error "ANSWER_FORMAT is not set in the config: the bridge's answer to a message, with a %s for the start of the message answered and a %s for the status"
#endif
#ifndef ANSWER_FIELD_COUNT
#error "ANSWER_FIELD_COUNT is not set in the config: how many fields of a message go into its answer, counting its letter as one"
#endif
#ifndef FIELD_SEPARATOR
#error "FIELD_SEPARATOR is not set in the config: the character between the fields of a message"
#endif

/**
 * @brief Starts the task that reads messages from the MCU and has each one dealt with.
 *
 * Needs uart_link_init() run first.
 */
void start_downstream_messaging(void);

/**
 * @brief Answers a handshake message from the MCU with how far the bridge can reach upstream.
 *
 * @param message The handshake message, ending in '\0'
 */
void handle_handshake(const char *message);

/**
 * @brief Answers a message from the MCU with how far the bridge can reach upstream right now:
 *        STATUS_OK, STATUS_CON_ERR_WIFI (no Wi-Fi) or STATUS_CON_ERR_BROKER (Wi-Fi, but no
 *        connection to the broker).
 *
 * @param message The message being answered, ending in '\0'
 */
void send_connection_status_downstream(const char *message);

/**
 * @brief Sends the MCU the bridge's answer to one of its messages, carrying a status.
 *
 * What the answer looks like is ANSWER_FORMAT in the config. The bridge fills in the start of the
 * message answered, its first ANSWER_FIELD_COUNT fields, and the text of the status.
 *
 * @param message The message being answered, ending in '\0'
 * @param status  The status to give
 */
void send_answer_downstream(const char *message, status_t status);

#endif
