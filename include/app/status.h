#ifndef STATUS_H
#define STATUS_H

#include "generated/config_in_use.h"
#include "mqtt_link.h"
#include "wifi_link.h"

/** What the bridge reports in an answer. */
typedef enum {
    STATUS_OK,              // Wi-Fi is up and the broker is connected
    STATUS_CON_ERR_WIFI,    // no Wi-Fi
    STATUS_CON_ERR_BROKER,  // Wi-Fi, but no connection to the broker
    STATUS_ERR_BROKER,      // the broker is connected, but did not confirm a message in time
    STATUS_ANS_ERR_LONG,    // the answer to a message was too long to send
} status_t;

// Whether each of the connection statuses is the case right now. Each is worked out from the two links at the place
// it is used, so it is always current.
#define STATUS_IS_OK             (check_wifi_link() && check_mqtt_link())
#define STATUS_IS_CON_ERR_WIFI   (!check_wifi_link())
#define STATUS_IS_CON_ERR_BROKER (check_wifi_link() && !check_mqtt_link())

// The text each status is sent as, in an answer: status_text[STATUS_OK] is STATUS_TEXT_OK from
// the config. There is no fallback.
#if !defined(STATUS_TEXT_OK) || !defined(STATUS_TEXT_CON_ERR_WIFI) || !defined(STATUS_TEXT_CON_ERR_BROKER) || !defined(STATUS_TEXT_ERR_BROKER) || !defined(STATUS_TEXT_ANS_ERR_LONG)
#error "STATUS_TEXT_OK, STATUS_TEXT_CON_ERR_WIFI, STATUS_TEXT_CON_ERR_BROKER, STATUS_TEXT_ERR_BROKER and STATUS_TEXT_ANS_ERR_LONG are not all set in the config: the text the bridge gives for each status"
#endif

extern const char *const status_text[];

#endif
