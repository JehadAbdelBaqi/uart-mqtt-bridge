#include "handshake.h"

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

// Built for a project: its config sets HANDSHAKE_RETRY_INTERVAL_MS, HANDSHAKE_CHECK_INTERVAL_MS
// and HANDSHAKE_MISSED_LIMIT.

static const char *TAG = "handshake";

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

#define REQUEST_LINE "H,request"
#define ACK_LINE     "H,ack"

// Written by the UART task (a line arrived) and by the handshake task (a request went unanswered)
static volatile bool connected = false;
static volatile bool awaiting_ack = false;  // a request has been sent and not answered yet
static volatile int missed = 0;             // requests in a row that went unanswered

/**
 * @brief Records whether the connection to the MCU is made, and shows it on the LED and in the log.
 *
 * Logs only when the state changes.
 *
 * @param now_connected true once the MCU has been heard from, false when it is lost
 */
static void set_connected(bool now_connected)
{
    bool changed = (now_connected != connected);

    connected = now_connected;
    led_show_mcu(now_connected ? LED_MCU_CONNECTED : LED_MCU_WAITING);

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
 * @brief Notes that the MCU has been heard from: the connection is made and nothing is missed.
 */
static void note_heard(void)
{
    missed = 0;
    awaiting_ack = false;
    set_connected(true);
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
 * @brief Task that sends "H,request": often while the connection is not made, then at the check interval.
 *
 * @param arg Not used
 */
static void handshake_task(void *arg)
{
    while (1) {
        if (awaiting_ack) {
            note_missed();
        }

        awaiting_ack = true;
        uart_link_send(REQUEST_LINE "\n");

        vTaskDelay(pdMS_TO_TICKS(connected ? HANDSHAKE_CHECK_INTERVAL_MS : HANDSHAKE_RETRY_INTERVAL_MS));
    }
}

void handshake_init(void)
{
    check_letter_not_routed();

    ESP_LOGI(TAG, "built for a project: waiting for the MCU");
    set_connected(connected);  // the MCU may already have been heard from; shows the state on the LED
    xTaskCreate(handshake_task, "handshake", 4096, NULL, 5, NULL);
}

bool handshake_handle_line(const char *line)
{
    if (line[0] != HANDSHAKE_LETTER) {
        return false;
    }

    if (strcmp(line, REQUEST_LINE) == 0) {
        uart_link_send(ACK_LINE "\n");
        note_heard();
        return true;
    }

    if (strcmp(line, ACK_LINE) == 0) {
        note_heard();
        return true;
    }

    ESP_LOGW(TAG, "line ignored: not a handshake line: %s", line);
    return true;
}

#endif
