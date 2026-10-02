#ifndef LED_H
#define LED_H

#include <stdint.h>

/**
 * @brief Sets up the RMT channel that drives the on-board RGB LED.
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

#endif
