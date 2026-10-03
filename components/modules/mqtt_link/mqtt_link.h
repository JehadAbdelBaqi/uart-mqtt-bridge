#ifndef MQTT_LINK_H
#define MQTT_LINK_H

/**
 * @brief Sets up the MQTT client for the broker in secrets/broker.h.
 *
 * Connects over TLS with the embedded certificates once Wi-Fi has an address.
 * Reconnecting after a drop happens in the background.
 * Needs NVS, the network interface layer and the default event loop started first (app_main does this).
 */
void mqtt_link_init(void);

#endif
