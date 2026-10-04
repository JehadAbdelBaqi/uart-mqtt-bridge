#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include <stdbool.h>

// First letter of the handshake lines. Reserved: a line starting with it is never published.
#define HANDSHAKE_LETTER 'H'

/**
 * @brief Starts the handshake with the MCU, if MCU_HANDSHAKE (config.h) switches it on.
 *
 * From then on the bridge sends "H,request" every HANDSHAKE_RETRY_INTERVAL_MS until the MCU
 * answers "H,ack", then every HANDSHAKE_CHECK_INTERVAL_MS to check the connection is still there.
 * After HANDSHAKE_MISSED_LIMIT requests in a row go unanswered the connection counts as lost.
 * The LED blinks red while the connection is not made.
 * Does nothing when MCU_HANDSHAKE is 0 or not set.
 * Needs led_startup() finished and uart_link_init() run first.
 */
void handshake_init(void);

/**
 * @brief Deals with a line from the MCU if it is a handshake line.
 *
 * "H,request" is answered with "H,ack"; either line counts the connection as made.
 * Every line starting with HANDSHAKE_LETTER is taken, even with the handshake off,
 * so none is ever published.
 *
 * @param line The line, ending in '\0'
 * @return true if the line was a handshake line and needs nothing more done with it
 */
bool handshake_handle_line(const char *line);

#endif
