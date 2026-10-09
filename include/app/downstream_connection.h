#ifndef DOWNSTREAM_CONNECTION_H
#define DOWNSTREAM_CONNECTION_H

// The connection to the MCU: whether it is there, worked out from how long it has been silent,
// and shown on the LED.
//
// The bridge never asks the MCU anything. The connection is made when the MCU's handshake message
// is answered, and every message from the MCU shows it is still there. Silent for
// MCU_QUIET_LIMIT_MS (config): the LED shows the MCU as quiet. Silent for as long again: the MCU
// counts as gone.
//
// A config that does not set MCU_QUIET_LIMIT_MS expects no MCU: nothing is watched, and the LED
// shows only Wi-Fi and the broker.

/**
 * @brief Starts watching for the MCU.
 *
 * Needs led_startup() finished first.
 */
void start_mcu_connection_watch(void);

/**
 * @brief Notes that a message has arrived from the MCU: it is still there.
 *
 * Does not make the connection; only an answered handshake does that.
 */
void note_mcu_heard(void);

/**
 * @brief Notes that the MCU's handshake message has been answered: the connection is made.
 */
void note_mcu_connected(void);

#endif
