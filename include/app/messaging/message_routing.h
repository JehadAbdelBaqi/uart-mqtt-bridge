#ifndef MESSAGE_ROUTING_H
#define MESSAGE_ROUTING_H

#include "generated/config_in_use.h"

// Message routing: where a message goes.

// The config for this build names the handshake's letter. There is no fallback.
#ifndef HANDSHAKE_LETTER
#error "HANDSHAKE_LETTER is not set in the config: the letter a handshake message from the MCU starts with"
#endif

/**
 * @brief Deals with a message received from downstream, from the MCU.
 *
 * A handshake message is answered by the bridge itself. Every other message is sent straight
 * upstream, as it is.
 *
 * @param message The message, ending in '\0'
 */
void handle_downstream_messages(const char *message);

#endif
