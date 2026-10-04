#ifndef CONFIG_H
#define CONFIG_H

// What the bridge does with messages: which topic each line from the MCU is
// published to, and which topics are passed down to the MCU.
// These are the values the bridge is tested with; a project replaces them with its own.

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

// Handshake with the MCU over the UART: the bridge expects a device there and checks that the
// two hear each other ("H,request" / "H,ack"). The LED blinks red until the connection is made.
// 1 switches it on; 0 switches it off, and the bridge takes no notice of whether a device is there.
#define MCU_HANDSHAKE 0

// While the connection is not made: how often "H,request" is repeated, in milliseconds
#define HANDSHAKE_RETRY_INTERVAL_MS 1000

// Once the connection is made: how often it is checked, in milliseconds
#define HANDSHAKE_CHECK_INTERVAL_MS 20000

// Checks in a row that go unanswered before the connection counts as lost
#define HANDSHAKE_MISSED_LIMIT 2

#endif
