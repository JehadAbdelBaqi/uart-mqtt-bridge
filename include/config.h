#ifndef CONFIG_H
#define CONFIG_H

// What the bridge does with messages: the settings of the UART link to the MCU, which topic
// each line from the MCU is published to, and which topics are passed down to the MCU.
// These are the values the bridge is tested with; a project replaces them with its own.

// The UART link to the MCU. Both ends have to use the same values.
#define UART_BAUD    115200  // bits per second; always 8 data bits, no parity, 1 stop bit
#define LINE_MAX_LEN 127     // longest line in either direction, in characters, not counting the '\n'

// Uplink: a line's first letter picks the topic it is published to.
// One { letter, topic } pair per line; every line but the last ends in a backslash.
#define UPLINK_ROUTES \
    { 'T', "bridge/test/up" }

// Downlink: every message on these topics is written to the MCU as one line.
// One topic per line, separated by commas; every line but the last ends in a backslash.
#define DOWNLINK_TOPICS \
    "bridge/test/down"

// Testing only: the bridge writes a numbered line to its own UART at this interval, in
// milliseconds, standing in for an MCU. Needs TX jumpered to RX. 0 switches it off.
#define DUMMY_LINE_INTERVAL_MS 2000

// Logs every line received from the MCU (up) and every line sent to it (down).
// 1 switches it on, 0 switches it off.
#define LOG_LINES 1

#endif
