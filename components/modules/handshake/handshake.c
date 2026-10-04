#include "handshake.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "generated/config_in_use.h"
#include "led.h"
#include "router.h"
#include "uart_link.h"

// Off unless the config switches it on
#ifndef MCU_HANDSHAKE
#define MCU_HANDSHAKE 0
#endif

// Used when the config leaves a timing out
#ifndef HANDSHAKE_RETRY_INTERVAL_MS
#define HANDSHAKE_RETRY_INTERVAL_MS 1000
#endif
#ifndef HANDSHAKE_CHECK_INTERVAL_MS
#define HANDSHAKE_CHECK_INTERVAL_MS 20000
#endif
#ifndef HANDSHAKE_MISSED_LIMIT
#define HANDSHAKE_MISSED_LIMIT 2
#endif

#define REQUEST_LINE "H,request"
#define ACK_LINE     "H,ack"

static const char *TAG = "handshake";

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
 * @brief Warns, once, that a handshake line arrived while the handshake is switched off.
 */
static void warn_handshake_off(void)
{
    static bool warned = false;

    if (warned) {
        return;
    }
    warned = true;
    ESP_LOGW(TAG, "handshake line from the MCU ignored: MCU_HANDSHAKE is off in config.h");
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
    if (router_find_topic(HANDSHAKE_LETTER) != NULL) {
        ESP_LOGE(TAG, "the routing table uses the letter '%c', which is reserved: those lines are never published", HANDSHAKE_LETTER);
    }

    if (!MCU_HANDSHAKE) {
        return;
    }

    ESP_LOGI(TAG, "handshake on: waiting for the MCU");
    set_connected(connected);  // the MCU may already have been heard from; shows the state on the LED
    xTaskCreate(handshake_task, "handshake", 4096, NULL, 5, NULL);
}

bool handshake_handle_line(const char *line)
{
    if (line[0] != HANDSHAKE_LETTER) {
        return false;
    }

    if (!MCU_HANDSHAKE) {
        warn_handshake_off();
        return true;
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
