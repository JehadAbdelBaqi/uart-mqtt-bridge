#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include <stdbool.h>

// First letter of the handshake lines. When the bridge is built for a project it is reserved:
// a line starting with it is never published.
#define HANDSHAKE_LETTER 'H'

/**
 * @brief Starts the handshake with the MCU, when the bridge is built for a project.
 *
 * Until the MCU answers, the bridge sends "H,request" every HANDSHAKE_REQUEST_INTERVAL_MS. Once the
 * connection is made it stays quiet while lines keep arriving from the MCU; after
 * HANDSHAKE_QUIET_LIMIT_MS with none it asks again at the same interval, and after
 * HANDSHAKE_MISSED_LIMIT requests in a row go unanswered the connection counts as lost.
 * The three values come from the project's config. The LED blinks red while the connection is not made.
 * Built standing alone, the bridge expects no MCU and runs no handshake.
 * Needs led_startup() finished, uart_link_init() and router_init() run first.
 */
void handshake_init(void);

/**
 * @brief Takes note of a line from the MCU, and deals with it if it is a handshake line.
 *
 * Any line shows the MCU is still there. "H,request" is answered with "H,ack"; either of the
 * two makes the connection.
 * Every line starting with HANDSHAKE_LETTER is taken, so none is ever published.
 * Built standing alone, the bridge takes no line: the letter is like any other.
 *
 * @param line The line, ending in '\0'
 * @return true if the line was a handshake line and needs nothing more done with it
 */
bool handshake_handle_line(const char *line);

#endif
