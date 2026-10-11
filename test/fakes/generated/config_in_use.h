// The config the unit tests are built with. It stands in for the header the build script writes
// (include/generated/config_in_use.h), and holds its own values so that a change to a real config
// does not change what the tests expect.

#define BUILT_FOR_PROJECT 1

#define UART_BAUD    115200
#define LINE_MAX_LEN 31  // short, so a test reaches the limit with a few characters

#define LINE_END_CHARACTERS     "\n"
#define LINE_SKIPPED_CHARACTERS "\r"

#define UPLINK_ROUTES \
    { 'W', "test/readings" }, \
    { 'R', "test/replies" }

#define DOWNLINK_TOPICS \
    "test/commands", \
    "test/acks"

#define HANDSHAKE_LETTER 'H'

#define ANSWER_FORMAT      "L,%s,%s"
#define ANSWER_FIELD_COUNT 2
#define FIELD_SEPARATOR    ','

#define STATUS_TEXT_OK             "ok"
#define STATUS_TEXT_CON_ERR_WIFI   "con_err_w"
#define STATUS_TEXT_CON_ERR_BROKER "con_err_br"
#define STATUS_TEXT_ERR_BROKER     "err_br"
#define STATUS_TEXT_ANS_ERR_LONG   "ans_err_long"

#define BROKER_CONFIRM_LIMIT_MS 1000
#define MCU_QUIET_LIMIT_MS      7000

#define LOG_LINES 1
