#include "uart_link.h"

#include <stdbool.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT   UART_NUM_1
#define UART_TX_PIN 7
#define UART_RX_PIN 6
#define UART_BAUD   115200

#define RX_BUFFER_SIZE 1024  // driver's receive buffer, bytes
#define LINE_MAX_LEN   128   // longest line, including the ending '\0'

static const char *TAG = "uart";

static void uart_task(void *arg)
{
    char line[LINE_MAX_LEN];
    size_t len = 0;
    bool too_long = false;
    uint8_t byte;

    while (1) {
        if (uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(100)) != 1) {
            continue;  // nothing arrived
        }

        if (byte == '\n') {
            if (too_long) {
                ESP_LOGW(TAG, "line dropped: longer than %d characters", LINE_MAX_LEN - 1);
            } else {
                line[len] = '\0';
                ESP_LOGI(TAG, "line: %s", line);
            }
            len = 0;
            too_long = false;
        } else if (byte == '\r') {
            // ignore: the '\n' that follows ends the line
        } else if (len < sizeof(line) - 1) {
            line[len++] = byte;
        } else {
            too_long = true;  // no room left: the whole line is dropped at its '\n'
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
    uart_write_bytes(UART_PORT, text, strlen(text));
}
