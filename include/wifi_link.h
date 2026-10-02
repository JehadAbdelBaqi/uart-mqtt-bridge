#ifndef WIFI_LINK_H
#define WIFI_LINK_H

/**
 * @brief Starts Wi-Fi in station mode with the network in secrets/wifi.h.
 *
 * Sets up what the Wi-Fi driver needs first (NVS, the network interface
 * layer, the default event loop), then switches the radio on.
 */
void wifi_link_init(void);

#endif
