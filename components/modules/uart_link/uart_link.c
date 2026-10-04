#include "uart_link.h"

#include <stdbool.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "config.h"
#include "router.h"

#define RX_BUFFER_SIZE  1024  // driver's receive buffer, bytes
#define READ_CHUNK_SIZE 64    // most bytes taken from the driver per read

// Off unless the config switches it on
#ifndef LOG_LINES
#define LOG_LINES 0
#endif

static const char *TAG = "uart";

// The line being collected; only the UART task uses these
static char line[LINE_MAX_LEN + 1];  // room for the ending '\0'
static size_t line_len = 0;
static bool line_too_long = false;

/**
 * @brief Logs one line received from the MCU or sent to it, if the config switches line logging on.
 *
 * @param direction "up" for a line from the MCU, "down" for one sent to it
 * @param text      The line, '\0'-terminated; a '\n' in it ends what is logged
 */
static void log_line(const char *direction, const char *text)
{
    if (!LOG_LINES) {
        return;
    }

    ESP_LOGI(TAG, "%s: %.*s", direction, (int)strcspn(text, "\n"), text);
}

/**
 * @brief Deals with a finished line: drops it if it was too long, otherwise passes it to the router.
 */
static void end_line(void)
{
    if (line_too_long) {
        ESP_LOGW(TAG, "line dropped: longer than %d characters", LINE_MAX_LEN);
        return;
    }

    line[line_len] = '\0';
    log_line("up", line);
    router_uplink(line);
}

/**
 * @brief Adds one received byte to the line being collected.
 *
 * '\n' ends the line and '\r' is ignored. Once a line is too long, the rest of it
 * is thrown away and the whole line is dropped at its '\n'.
 *
 * @param byte The byte received
 */
static void handle_byte(uint8_t byte)
{
    switch (byte) {
    case '\n':
        end_line();
        line_len = 0;
        line_too_long = false;
        break;
    case '\r':
        break;  // ignored: the '\n' that follows ends the line
    default:
        if (line_len == sizeof(line) - 1) {
            line_too_long = true;  // no room left
            break;
        }
        line[line_len++] = byte;
        break;
    }
}

/**
 * @brief Task that reads the UART and hands every byte to handle_byte().
 *
 * Sleeps until a byte arrives, then takes whatever else is already waiting in the
 * driver, so a short line is handled at once and a burst comes out in one read.
 *
 * @param arg Not used
 */
static void uart_task(void *arg)
{
    uint8_t chunk[READ_CHUNK_SIZE];

    while (1) {
        int count = uart_read_bytes(UART_PORT, chunk, 1, portMAX_DELAY);  // wait for the first byte
        if (count <= 0) {
            continue;
        }

        int more = uart_read_bytes(UART_PORT, chunk + 1, sizeof(chunk) - 1, 0);  // whatever else is there, no waiting
        if (more > 0) {
            count += more;
        }

        for (int i = 0; i < count; i++) {
            handle_byte(chunk[i]);
        }
    }
}

void uart_link_init(void)
{
    uart_config_t config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));  // TX, RX, RTS, CTS
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, RX_BUFFER_SIZE, 0, 0, NULL, 0));

    xTaskCreate(uart_task, "uart", 4096, NULL, 5, NULL);
}

void uart_link_send(const char *text)
{
    if (uart_write_bytes(UART_PORT, text, strlen(text)) < 0) {
        ESP_LOGW(TAG, "send failed");
        return;
    }

    log_line("down", text);
}
