#ifndef DUMMY_SOURCE_H
#define DUMMY_SOURCE_H

/**
 * @brief Starts the dummy data source, which stands in for an MCU during testing.
 *
 * Every DUMMY_LINE_INTERVAL_MS (config.h) it writes one line to the bridge's own UART:
 * "T,<sequence number>,<milliseconds since start>". With TX jumpered to RX the line comes
 * back in and travels up like a line from a real MCU.
 * Does nothing when DUMMY_LINE_INTERVAL_MS is 0 or not set.
 * Needs uart_link_init() run first.
 */
void dummy_source_init(void);

#endif
