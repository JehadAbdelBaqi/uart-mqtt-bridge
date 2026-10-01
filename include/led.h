#ifndef LED_H
#define LED_H

#include <stdint.h>

void led_init(void);
void led_set(uint8_t red, uint8_t green, uint8_t blue);
void led_startup(void);

#endif
