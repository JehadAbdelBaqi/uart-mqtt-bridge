#include "led.h"

#include <stdbool.h>

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO 21  // on-board RGB LED

#define STARTUP_FAST_MS 265   // per colour, two quick passes
#define STARTUP_SLOW_MS 1000  // per colour, one slow pass

#define LINK_FLASH_MS 250  // amber on / off time while connecting

enum { RED, AMBER, GREEN, BLUE };

static const uint8_t colours[4][3] = {
    [RED]   = { 40, 0, 0 },
    [AMBER] = { 40, 20, 0 },
    [GREEN] = { 0, 40, 0 },
    [BLUE]  = { 0, 0, 40 },
};

static rmt_channel_handle_t led_channel;
static rmt_encoder_handle_t led_encoder;

static volatile led_link_state_t link_state = LED_LINK_NONE;

/**
 * @brief Task that keeps the LED showing the link state.
 *
 * Down: red. Connecting: amber, flashing. Up: green.
 * Does nothing until led_show_link() is first called, so led_startup() has the LED to itself.
 *
 * @param arg Not used
 */
static void led_link_task(void *arg)
{
    bool flash_on = false;

    while (1) {
        if (link_state == LED_LINK_DOWN) {
            led_set(colours[RED][0], colours[RED][1], colours[RED][2]);
        } else if (link_state == LED_LINK_CONNECTING) {
            flash_on = !flash_on;
            if (flash_on) {
                led_set(colours[AMBER][0], colours[AMBER][1], colours[AMBER][2]);
            } else {
                led_set(0, 0, 0);
            }
        } else if (link_state == LED_LINK_UP) {
            led_set(colours[GREEN][0], colours[GREEN][1], colours[GREEN][2]);
        }
        vTaskDelay(pdMS_TO_TICKS(LINK_FLASH_MS));
    }
}

void led_init(void)
{
    rmt_tx_channel_config_t channel_config = {
        .gpio_num = LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,  // 1 count = 0.1 us
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&channel_config, &led_channel));

    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = { .level0 = 1, .duration0 = 3, .level1 = 0, .duration1 = 9 },  // 0: 0.3 us high, 0.9 us low
        .bit1 = { .level0 = 1, .duration0 = 9, .level1 = 0, .duration1 = 3 },  // 1: 0.9 us high, 0.3 us low
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(led_channel));

    xTaskCreate(led_link_task, "led", 2048, NULL, 5, NULL);
}

void led_set(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t bytes[3] = { green, red, blue };  // the LED takes green first
    rmt_transmit_config_t transmit_config = { 0 };

    ESP_ERROR_CHECK(rmt_transmit(led_channel, led_encoder, bytes, sizeof(bytes), &transmit_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY));
}

/**
 * @brief Shows each of the four colours in turn.
 *
 * @param ms_per_colour How long each colour stays on, in milliseconds
 */
static void led_pass(uint32_t ms_per_colour)
{
    for (int i = 0; i < 4; i++) {
        led_set(colours[i][0], colours[i][1], colours[i][2]);
        vTaskDelay(pdMS_TO_TICKS(ms_per_colour));
    }
}

void led_startup(void)
{
    led_pass(STARTUP_FAST_MS);
    led_pass(STARTUP_FAST_MS);
    led_pass(STARTUP_SLOW_MS);
    led_set(0, 0, 0);  // off
}

void led_show_link(led_link_state_t state)
{
    link_state = state;
}
