#ifndef UART_LINK_H
#define UART_LINK_H

// Longest line in either direction, in characters, not counting the '\n'
#define LINE_MAX_LEN 127

/**
 * @brief Sets up the UART to the MCU and starts the task that reads lines from it.
 *
 * Port, pins and baud rate are set in board.h.
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
