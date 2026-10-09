#include "downstream_connection.h"

#include "generated/config_in_use.h"

#ifndef MCU_QUIET_LIMIT_MS

// No MCU is expected: nothing is watched.

void start_mcu_connection_watch(void)
{
}

void note_mcu_heard(void)
{
}

void note_mcu_connected(void)
{
}

#else

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "led.h"

#define WATCH_INTERVAL_US (100 * 1000)  // how often the MCU's silence is checked: every 100 ms

static const char *TAG = "mcu";

// Written by the task that reads the UART (a message arrived) and by the watch timer (the MCU has
// been silent too long)
static volatile bool mcu_connected = false;
static volatile uint32_t last_heard_ms = 0;  // when the last message arrived from the MCU

static esp_timer_handle_t watch_timer;

/**
 * @brief Gives the time since the bridge started, in milliseconds.
 */
static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/**
 * @brief Records whether the connection to the MCU is made, and logs it when it changes.
 *
 * @param now_connected true once the MCU's handshake has been answered, false when it is gone
 */
static void set_mcu_connected(bool now_connected)
{
    bool changed = (now_connected != mcu_connected);

    mcu_connected = now_connected;

    if (!changed) {
        return;
    }
    if (now_connected) {
        ESP_LOGI(TAG, "connection to the MCU made");
        return;
    }
    ESP_LOGW(TAG, "connection to the MCU lost");
}

/**
 * @brief Works out the state of the connection to the MCU from how long it has been silent.
 *
 * @return Waiting while the connection is not made, or once the MCU has been silent for twice the
 *         quiet limit; checking once it has been silent for the quiet limit; otherwise connected
 */
static led_mcu_state_t mcu_state(void)
{
    uint32_t silent_ms = now_ms() - last_heard_ms;

    if (!mcu_connected) {
        return LED_MCU_WAITING;
    }
    if (silent_ms >= 2 * MCU_QUIET_LIMIT_MS) {
        return LED_MCU_WAITING;
    }
    if (silent_ms >= MCU_QUIET_LIMIT_MS) {
        return LED_MCU_CHECKING;
    }
    return LED_MCU_CONNECTED;
}

/**
 * @brief Called by the watch timer: shows the state of the connection to the MCU on the LED, and
 *        counts the MCU as gone once it has been silent for too long.
 *
 * @param arg Not used
 */
static void watch_mcu(void *arg)
{
    led_mcu_state_t state = mcu_state();

    if (state == LED_MCU_WAITING) {
        set_mcu_connected(false);
    }
    led_show_mcu(state);
}

void start_mcu_connection_watch(void)
{
    esp_timer_create_args_t watch_timer_args = {
        .callback = watch_mcu,
        .name = "mcu_watch",
    };

    ESP_LOGI(TAG, "waiting for the MCU");
    ESP_ERROR_CHECK(esp_timer_create(&watch_timer_args, &watch_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(watch_timer, WATCH_INTERVAL_US));
}

void note_mcu_heard(void)
{
    last_heard_ms = now_ms();
}

void note_mcu_connected(void)
{
    last_heard_ms = now_ms();
    set_mcu_connected(true);
}

#endif
