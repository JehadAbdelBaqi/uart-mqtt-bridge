#ifndef FAKE_ESP_TIMER_H
#define FAKE_ESP_TIMER_H

#include <stdint.h>

// Stands in for ESP-IDF's esp_timer.h. A timer never runs out by itself: a test makes it, with
// fake_timer_fire() in fakes.h. The time is whatever a test sets with fake_set_time_ms().

typedef struct fake_timer *esp_timer_handle_t;

typedef struct {
    void (*callback)(void *arg);
    void *arg;
    const char *name;
} esp_timer_create_args_t;

int esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *timer);
int esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us);
int esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us);
int esp_timer_stop(esp_timer_handle_t timer);
int64_t esp_timer_get_time(void);

#endif
