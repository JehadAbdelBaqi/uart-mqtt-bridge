#ifndef WIFI_LINK_H
#define WIFI_LINK_H

/**
 * @brief Starts Wi-Fi in station mode with the network in secrets/wifi.h.
 *
 * Switches the radio on; connecting, and connecting again after a drop, happen
 * in the background. Needs NVS, the network interface layer and the default
 * event loop started first (app_main does this).
 */
void wifi_link_init(void);

#endif
