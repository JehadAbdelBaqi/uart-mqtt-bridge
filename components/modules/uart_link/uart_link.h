#ifndef UART_LINK_H
#define UART_LINK_H

#include "generated/config_in_use.h"

// The config for this build sets the link's two values. There is no fallback for either.
#ifndef UART_BAUD
#error "UART_BAUD is not set in the config: the baud rate of the UART link to the MCU"
#endif
#ifndef LINE_MAX_LEN
#error "LINE_MAX_LEN is not set in the config: the longest line, in characters, not counting the '\n'"
#endif

/**
 * @brief Sets up the UART to the MCU and starts the task that reads lines from it.
 *
 * Port and pins are set in board.h; the baud rate and the line limit in the config.
 */
void uart_link_init(void);

/**
 * @brief Writes text to the MCU unchanged.
 *
 * Nothing is added: to send a line, end the text with '\n'.
 *
 * @param text Text to send, '\0'-terminated
 */
void uart_link_send(const char *text);

#endif
