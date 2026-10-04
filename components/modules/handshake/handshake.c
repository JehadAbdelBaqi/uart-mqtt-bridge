#include "handshake.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "generated/config_in_use.h"
#include "led.h"
#include "router.h"
#include "uart_link.h"

#if !BUILT_FOR_PROJECT

// The bridge standing alone: no MCU is expected, so there is no handshake in the firmware.

void handshake_init(void)
{
}

bool handshake_handle_line(const char *line)
{
    return false;
}

#else

// Built for a project: its config sets HANDSHAKE_REQUEST_INTERVAL_MS, HANDSHAKE_QUIET_LIMIT_MS
// and HANDSHAKE_MISSED_LIMIT.

static const char *TAG = "handshake";

#define REQUEST_LINE "H,request"
#define ACK_LINE     "H,ack"

// Written by the UART task (a line arrived) and by the handshake task (a request went unanswered)
static volatile bool connected = false;
static volatile bool awaiting_ack = false;   // a request has been sent and not answered yet
static volatile int missed = 0;              // requests in a row that went unanswered
static volatile uint32_t last_heard_ms = 0;  // when the last line arrived from the MCU

/**
 * @brief Gives the time since the bridge started, in milliseconds.
 */
static uint32_t now_ms(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

/**
 * @brief Logs an error if the routing table uses the handshake's letter, which is never published.
 */
static void check_letter_not_routed(void)
{
    if (router_find_topic(HANDSHAKE_LETTER) == NULL) {
        return;
    }
    ESP_LOGE(TAG, "the routing table uses the letter '%c', which is reserved: those lines are never published", HANDSHAKE_LETTER);
}

/**
 * @brief Shows the state of the connection to the MCU on the LED.
 *
 * Not made: waiting. Made, with a request out that is not yet answered: checking. Otherwise connected.
 */
static void show_on_led(void)
{
    if (!connected) {
        led_show_mcu(LED_MCU_WAITING);
        return;
    }
    led_show_mcu(awaiting_ack ? LED_MCU_CHECKING : LED_MCU_CONNECTED);
}

/**
 * @brief Records whether the connection to the MCU is made, and shows it on the LED and in the log.
 *
 * Logs only when the state changes.
 *
 * @param now_connected true once the MCU has answered, false when it is lost
 */
static void set_connected(bool now_connected)
{
    bool changed = (now_connected != connected);

    connected = now_connected;
    show_on_led();

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
 * @brief Notes that a line has arrived from the MCU: it is still there, and nothing is missed.
 *
 * Does not make the connection; only a handshake line does that.
 */
static void note_heard(void)
{
    last_heard_ms = now_ms();
    missed = 0;
    awaiting_ack = false;
    show_on_led();
}

/**
 * @brief Notes that the last request went unanswered; after the limit, the connection is lost.
 */
static void note_missed(void)
{
    if (!connected) {
        return;  // still trying to make the connection: nothing to lose
    }

    missed++;
    if (missed < HANDSHAKE_MISSED_LIMIT) {
        return;
    }
    set_connected(false);
}

/**
 * @brief Gives how long the bridge can stay quiet before it has to ask, in milliseconds.
 *
 * @return The time left of the quiet limit while the connection is made and the MCU has been
 *         heard from within it; 0 when a request is due
 */
static uint32_t quiet_time_left_ms(void)
{
    uint32_t quiet_ms = now_ms() - last_heard_ms;

    if (!connected) {
        return 0;
    }
    if (quiet_ms >= HANDSHAKE_QUIET_LIMIT_MS) {
        return 0;
    }
    return HANDSHAKE_QUIET_LIMIT_MS - quiet_ms;
}

/**
 * @brief Task that asks the MCU whether it is there, with "H,request", only when it has to.
 *
 * While the connection is not made, it asks every HANDSHAKE_REQUEST_INTERVAL_MS. Once it is made,
 * it stays quiet for as long as lines keep arriving from the MCU; after HANDSHAKE_QUIET_LIMIT_MS
 * with none, it asks at the same interval, and after HANDSHAKE_MISSED_LIMIT requests in a row go
 * unanswered the connection is lost.
 *
 * @param arg Not used
 */
static void handshake_task(void *arg)
{
    while (1) {
        uint32_t quiet_left_ms = quiet_time_left_ms();

        if (quiet_left_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(quiet_left_ms) + 1);  // + 1 tick: never a delay of nothing
            continue;
        }

        if (awaiting_ack) {
            note_missed();
        }

        awaiting_ack = true;
        show_on_led();
        uart_link_send(REQUEST_LINE "\n");
        vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_REQUEST_INTERVAL_MS));
    }
}

void handshake_init(void)
{
    check_letter_not_routed();

    ESP_LOGI(TAG, "built for a project: waiting for the MCU");
    set_connected(connected);  // the MCU may already have answered; shows the state on the LED
    xTaskCreate(handshake_task, "handshake", 4096, NULL, 5, NULL);
}

bool handshake_handle_line(const char *line)
{
    note_heard();

    if (line[0] != HANDSHAKE_LETTER) {
        return false;
    }

    if (strcmp(line, REQUEST_LINE) == 0) {
        uart_link_send(ACK_LINE "\n");
        set_connected(true);
        return true;
    }

    if (strcmp(line, ACK_LINE) == 0) {
        set_connected(true);
        return true;
    }

    ESP_LOGW(TAG, "line ignored: not a handshake line: %s", line);
    return true;
}

#endif
