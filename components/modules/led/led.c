#include "led.h"

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"

#define STARTUP_FAST_MS 265   // per colour, two quick passes
#define STARTUP_SLOW_MS 1000  // per colour, one slow pass

// On / off times of the blinks, in ticks of the LED task
#define LINK_TICK_MS      50
#define MCU_CHECK_TICKS   2    // green while a quiet MCU is being asked: 100 ms
#define CONNECTING_TICKS  5    // amber while Wi-Fi is connecting: 250 ms
#define BROKER_WAIT_TICKS 10   // green while the broker isn't connected, red while waiting for the MCU: 500 ms

// The LED reads each bit from how long the signal stays high and then low:
// 0 = short high, long low; 1 = long high, short low.
#define RMT_RESOLUTION_HZ 10000000  // the RMT counts in steps of 0.1 us
#define BIT_SHORT_NS      300
#define BIT_LONG_NS       900
#define NS_TO_COUNTS(ns)  ((ns) / (1000000000 / RMT_RESOLUTION_HZ))  // a time in ns as RMT steps

enum { RED, AMBER, GREEN, BLUE, COLOUR_COUNT };

static const uint8_t colours[COLOUR_COUNT][3] = {
    [RED]   = { 40, 0, 0 },
    [AMBER] = { 40, 20, 0 },
    [GREEN] = { 0, 40, 0 },
    [BLUE]  = { 0, 0, 40 },
};

static rmt_channel_handle_t led_channel;
static rmt_encoder_handle_t led_encoder;

static volatile led_wifi_state_t wifi_state = LED_WIFI_NONE;
static volatile bool broker_connected = false;
static volatile led_mcu_state_t mcu_state = LED_MCU_NONE;

/**
 * @brief Switches the LED off.
 */
static void led_off(void)
{
    led_set(0, 0, 0);
}

/**
 * @brief Shows one of the named colours, or switches the LED off.
 *
 * @param colour RED, AMBER, GREEN or BLUE
 * @param lit    true to show the colour, false for off
 */
static void led_show_colour(int colour, bool lit)
{
    if (!lit) {
        led_off();
        return;
    }
    led_set(colours[colour][0], colours[colour][1], colours[colour][2]);
}

/**
 * @brief Says whether a blink is in its lit half.
 *
 * @param tick         The LED task's tick count
 * @param ticks_per_on How many ticks the blink stays on, and then off
 * @return true while it is lit
 */
static bool blink_on(uint32_t tick, uint32_t ticks_per_on)
{
    return (tick / ticks_per_on) % 2 == 0;
}

/**
 * @brief Shows the Wi-Fi and broker state.
 *
 * Wi-Fi down: red. Wi-Fi connecting: amber, flashing.
 * Wi-Fi up, broker not connected: green, blinking once a second. Both up: solid green.
 *
 * @param tick The LED task's tick count
 */
static void led_show_upstream(uint32_t tick)
{
    bool fast_flash_on = blink_on(tick, CONNECTING_TICKS);
    bool slow_blink_on = blink_on(tick, BROKER_WAIT_TICKS);

    switch (wifi_state) {
    case LED_WIFI_DOWN:
        led_show_colour(RED, true);
        break;
    case LED_WIFI_CONNECTING:
        led_show_colour(AMBER, fast_flash_on);
        break;
    case LED_WIFI_UP:
        led_show_colour(GREEN, broker_connected || slow_blink_on);  // solid once the broker is connected
        break;
    case LED_WIFI_NONE:
        break;  // not following the link yet
    }
}

/**
 * @brief Shows the state that comes first: the connection to the MCU, otherwise Wi-Fi and the broker.
 *
 * Waiting for the MCU: red, blinking once a second. Checking a quiet MCU: green, flashing every 100 ms.
 * Both are shown in place of the Wi-Fi and broker states.
 *
 * @param tick The LED task's tick count
 */
static void led_show_state(uint32_t tick)
{
    if (mcu_state == LED_MCU_WAITING) {
        led_show_colour(RED, blink_on(tick, BROKER_WAIT_TICKS));
        return;
    }

    if (mcu_state == LED_MCU_CHECKING) {
        led_show_colour(GREEN, blink_on(tick, MCU_CHECK_TICKS));
        return;
    }

    led_show_upstream(tick);
}

/**
 * @brief Task that keeps the LED showing the state of the bridge's connections.
 *
 * Does nothing until led_show_wifi() or led_show_mcu() is first called, so led_startup()
 * has the LED to itself.
 *
 * @param arg Not used
 */
static void led_link_task(void *arg)
{
    uint32_t tick = 0;

    while (1) {
        led_show_state(tick);

        tick++;
        vTaskDelay(pdMS_TO_TICKS(LINK_TICK_MS));
    }
}

void led_init(void)
{
    rmt_tx_channel_config_t channel_config = {
        .gpio_num = LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&channel_config, &led_channel));

    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = { .level0 = 1, .duration0 = NS_TO_COUNTS(BIT_SHORT_NS), .level1 = 0, .duration1 = NS_TO_COUNTS(BIT_LONG_NS) },
        .bit1 = { .level0 = 1, .duration0 = NS_TO_COUNTS(BIT_LONG_NS), .level1 = 0, .duration1 = NS_TO_COUNTS(BIT_SHORT_NS) },
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
    for (int i = 0; i < COLOUR_COUNT; i++) {
        led_show_colour(i, true);
        vTaskDelay(pdMS_TO_TICKS(ms_per_colour));
    }
}

void led_startup(void)
{
    led_pass(STARTUP_FAST_MS);
    led_pass(STARTUP_FAST_MS);
    led_pass(STARTUP_SLOW_MS);
    led_off();
}

void led_show_wifi(led_wifi_state_t state)
{
    if (state == LED_WIFI_DOWN || state == LED_WIFI_CONNECTING) {
        broker_connected = false;  // no broker connection without Wi-Fi
    }
    wifi_state = state;
}

void led_show_broker(bool connected)
{
    broker_connected = connected;
}

void led_show_mcu(led_mcu_state_t state)
{
    mcu_state = state;
}
