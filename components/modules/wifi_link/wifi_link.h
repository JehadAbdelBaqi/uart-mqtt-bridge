#ifndef WIFI_LINK_H
#define WIFI_LINK_H

#include <stdbool.h>

/**
 * @brief Starts Wi-Fi in station mode with the network in secrets/wifi.h.
 *
 * Switches the radio on; connecting, and connecting again after a drop, happen
 * in the background. Needs NVS, the network interface layer and the default
 * event loop started first (app_main does this).
 */
void wifi_link_init(void);

/**
 * @brief Says whether Wi-Fi is connected and has an address.
 *
 * @return true from the moment an address is received until the connection drops
 */
bool check_wifi_link(void);

#endif
