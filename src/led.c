#include "led.h"

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO 21  // on-board RGB LED

#define STARTUP_FAST_MS 265   // per colour, two quick passes
#define STARTUP_SLOW_MS 1000  // per colour, one slow pass

// red, amber, green, blue
static const uint8_t colours[4][3] = {
    { 40, 0, 0 },
    { 40, 20, 0 },
    { 0, 40, 0 },
    { 0, 0, 40 },
};

static rmt_channel_handle_t led_channel;
static rmt_encoder_handle_t led_encoder;

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
}

void led_set(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t bytes[3] = { green, red, blue };  // the LED takes green first
    rmt_transmit_config_t transmit_config = { 0 };

    ESP_ERROR_CHECK(rmt_transmit(led_channel, led_encoder, bytes, sizeof(bytes), &transmit_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY));
}

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
