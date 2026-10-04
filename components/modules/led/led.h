#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

/** The Wi-Fi state the LED shows once it is set to follow the link. */
typedef enum {
    LED_WIFI_NONE,        // not following the link yet: LED left alone
    LED_WIFI_DOWN,        // red
    LED_WIFI_CONNECTING,  // amber, flashing
    LED_WIFI_UP,          // green: blinking until the broker is connected, then solid
} led_wifi_state_t;

/** Whether an MCU is expected on the UART, and if so whether the connection to it is made. */
typedef enum {
    LED_MCU_NONE,       // no MCU expected: the LED shows only Wi-Fi and the broker
    LED_MCU_WAITING,    // red, blinking; shown in place of the Wi-Fi and broker states
    LED_MCU_CONNECTED,  // the LED shows Wi-Fi and the broker
} led_mcu_state_t;

/**
 * @brief Sets up the RMT channel that drives the on-board RGB LED,
 *        and starts the task that shows the link state.
 *
 * Call once before any other led_ function.
 */
void led_init(void);

/**
 * @brief Sets the LED to a colour and waits until the LED has received it.
 *
 * @param red   Red brightness, 0-255
 * @param green Green brightness, 0-255
 * @param blue  Blue brightness, 0-255
 */
void led_set(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Plays the startup sequence, then switches the LED off.
 *
 * Red, amber, green, blue: two quick passes, then one slow pass (about 6 s).
 * Blocks until it is finished.
 */
void led_startup(void);

/**
 * @brief Sets the Wi-Fi state the LED shows from now on.
 *
 * Call after led_startup(): from the first call, the LED follows the link.
 * Down or connecting also counts the broker as disconnected.
 *
 * @param state Down, connecting or up
 */
void led_show_wifi(led_wifi_state_t state);

/**
 * @brief Sets whether the broker is connected, which decides solid or blinking green.
 *
 * @param connected true once connected to the broker, false when the connection drops
 */
void led_show_broker(bool connected);

/**
 * @brief Sets the state of the connection to the MCU that the LED shows from now on.
 *
 * While it is LED_MCU_WAITING the LED blinks red, whatever Wi-Fi and the broker are doing.
 * Call after led_startup().
 *
 * @param state None expected, waiting or connected
 */
void led_show_mcu(led_mcu_state_t state);

#endif
