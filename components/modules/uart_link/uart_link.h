#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Sets up the UART to the MCU for sending and receiving.
 *
 * Port and pins are set in board.h; the baud rate in the config (UART_BAUD), always with 8 data
 * bits, no parity and 1 stop bit.
 * Call once, before uart_link_read() and uart_link_send().
 */
void uart_link_init(void);

/**
 * @brief Waits until at least one byte has arrived, then gives every byte that is waiting.
 *
 * Sleeps while nothing arrives, so a task can call it in a loop.
 *
 * @param bytes Filled in with the bytes received
 * @param size  The most bytes to give, at least 1
 * @return How many bytes were given
 */
size_t uart_link_read(uint8_t *bytes, size_t size);

/**
 * @brief Writes text to the UART unchanged.
 *
 * @param text Text to send, '\0'-terminated
 * @return true if the text was written
 */
bool uart_link_send(const char *text);

#endif
