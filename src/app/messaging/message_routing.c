#include "message_routing.h"

#include <stdbool.h>

#include "downstream_messaging.h"
#include "helpers.h"
#include "downstream_connection.h"
#include "upstream_messaging.h"

/**
 * @brief Says whether a message from the MCU is a handshake message.
 *
 * It is one that starts with HANDSHAKE_LETTER from the config.
 *
 * @param message The message, ending in '\0'
 * @return true if it is a handshake message
 */
static bool is_handshake(const char *message)
{
    return message[0] == HANDSHAKE_LETTER;
}

void handle_downstream_messages(const char *message)
{
    note_mcu_heard();
    log_message("up", message);

    if (is_handshake(message)) {
        handle_handshake(message);
        return;
    }

    send_message_upstream(message);
}
