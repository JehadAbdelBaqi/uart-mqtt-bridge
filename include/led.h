#ifndef LED_H
#define LED_H

#include <stdint.h>

/** What the LED shows once it is set to follow the link. */
typedef enum {
    LED_LINK_NONE,        // not following the link yet: LED left alone
    LED_LINK_DOWN,        // red
    LED_LINK_CONNECTING,  // amber, flashing
    LED_LINK_UP,          // green
} led_link_state_t;

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
 * @brief Sets the link state the LED shows from now on.
 *
 * Call after led_startup(): from the first call, the LED follows the link.
 *
 * @param state Down, connecting or up
 */
void led_show_link(led_link_state_t state);

#endif
