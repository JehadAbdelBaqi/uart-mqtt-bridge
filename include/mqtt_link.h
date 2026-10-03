#ifndef MQTT_LINK_H
#define MQTT_LINK_H

/**
 * @brief Sets up the MQTT client for the broker in secrets/broker.h.
 *
 * Connects over TLS with the embedded certificates once Wi-Fi has an address.
 * Reconnecting after a drop happens in the background.
 * Call after wifi_link_init(), which creates the event loop this listens on.
 */
void mqtt_link_init(void);

#endif
