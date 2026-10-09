#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>
#include <stddef.h>

#include "generated/config_in_use.h"

// The config for this build says what ends a line. There is no fallback.
#ifndef LINE_END_CHARACTERS
#error "LINE_END_CHARACTERS is not set in the config: the characters that end a line, as text"
#endif

// Room for a message made ready to send: the message, then the line end and '\0'
#define DOWNSTREAM_LINE_SIZE (LINE_MAX_LEN + 2)

/**
 * @brief Checks that a message is one the bridge can pass on, in either direction.
 *
 * Why: both ends work with one line limit, LINE_MAX_LEN. A longer message would be dropped by
 * whoever receives it, and an empty one carries nothing, so neither is sent on.
 *
 * @param length How many bytes the message has
 * @return true if the message is not empty and not longer than LINE_MAX_LEN
 */
bool message_is_valid(size_t length);

/**
 * @brief Writes a message to the log, if the config switches message logging on (LOG_LINES).
 *
 * Why: to watch the traffic through the bridge on the serial monitor.
 *
 * @param direction "up" for a message from the MCU, "down" for one sent to it
 * @param message   The message, ending in '\0'; a line end in it ends what is logged
 */
void log_message(const char *direction, const char *message);

/**
 * @brief Makes a message ready to send downstream, to the MCU: the message, then the line end,
 *        then '\0'.
 *
 * The line end is the first character of LINE_END_CHARACTERS in the config.
 *
 * Why: the MCU reads bytes from the UART and only knows a message is complete when it sees the
 * line end. A message from the broker has no line end and no '\0', and one the bridge builds has
 * no line end, so the two are added here, whichever of them the message came from. The '\0' is
 * for the function that sends the text, to know where it stops; it is not sent.
 *
 * @param message The message's bytes; it need not end in '\0'
 * @param length  How many bytes the message has
 * @param line    Filled in with the message, the line end and '\0'; DOWNSTREAM_LINE_SIZE characters
 */
void format_message_for_downstream(const char *message, size_t length, char *line);

#endif
