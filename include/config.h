#ifndef CONFIG_H
#define CONFIG_H

// What the bridge does with messages: the settings of the UART link to the MCU, which topic
// each line from the MCU is published to, and which topics are passed down to the MCU.
// These are the values the bridge is tested with; a project replaces them with its own.

// The UART link to the MCU. Both ends have to use the same values.
#define UART_BAUD    115200  // bits per second; always 8 data bits, no parity, 1 stop bit
#define LINE_MAX_LEN 127     // longest line in either direction, in characters, not counting the '\n'

// How the bytes from the MCU are cut into messages. Each is a list of characters, as text.
#define LINE_END_CHARACTERS     "\n"  // any of these ends a message; the bridge ends its own with the first
#define LINE_SKIPPED_CHARACTERS "\r"  // any of these is left out, as if it was not sent

// Uplink: a line's first letter picks the topic it is published to.
// One { letter, topic } pair per line; every line but the last ends in a backslash.
#define UPLINK_ROUTES \
    { 'T', "bridge/test/up" }

// Downlink: every message on these topics is written to the MCU as one line.
// One topic per line, separated by commas; every line but the last ends in a backslash.
#define DOWNLINK_TOPICS \
    "bridge/test/down"

// The letter a handshake message from the MCU starts with. The bridge answers such a message
// itself, with its status, and never publishes it. A project sets its own.
#define HANDSHAKE_LETTER 'H'

// The bridge's answer to a message from the MCU. The bridge fills in two things: the start of
// the message it answers, and its status. Its letter has no route above, so an answer that comes
// back in through the test jumper is not published and not answered again. A project sets its own.
#define ANSWER_FORMAT      "L,%s,%s"
#define ANSWER_FIELD_COUNT 2    // how many fields of the message make its start, counting its letter as one
#define FIELD_SEPARATOR    ','  // between the fields of a message

// The text the bridge gives for each status, in an answer. A project sets its own.
#define STATUS_TEXT_OK             "ok"            // Wi-Fi is up and the broker is connected
#define STATUS_TEXT_CON_ERR_WIFI   "con_err_w"     // no Wi-Fi
#define STATUS_TEXT_CON_ERR_BROKER "con_err_br"    // Wi-Fi, but no connection to the broker
#define STATUS_TEXT_ERR_BROKER     "err_br"        // the broker is connected, but did not confirm a message in time
#define STATUS_TEXT_ANS_ERR_LONG   "ans_err_long"  // the answer to a message was too long to send

// How long the bridge waits for the broker to confirm a line it has published, in milliseconds,
// before it answers the MCU that the line has not gone through. A project works this out from
// its MCU's own time limit; standing alone, the bridge uses this value.
#define BROKER_CONFIRM_LIMIT_MS 2000

// Testing only: the bridge writes a numbered line to its own UART at this interval, in
// milliseconds, standing in for an MCU. Needs TX jumpered to RX. 0 switches it off.
#define DUMMY_LINE_INTERVAL_MS 2000

// Logs every line received from the MCU (up) and every line sent to it (down).
// 1 switches it on, 0 switches it off.
#define LOG_LINES 1

#endif
