#include "uart_link.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "board.h"
#include "generated/config_in_use.h"

// The config for this build sets the baud rate. There is no fallback.
#ifndef UART_BAUD
#error "UART_BAUD is not set in the config: the baud rate of the UART link to the MCU"
#endif

#define RX_BUFFER_SIZE 1024  // driver's receive buffer, bytes

static const char *TAG = "uart";

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
}

size_t uart_link_read(uint8_t *bytes, size_t size)
{
    int count = uart_read_bytes(UART_PORT, bytes, 1, portMAX_DELAY);  // wait for the first byte
    if (count <= 0) {
        return 0;
    }

    int more = uart_read_bytes(UART_PORT, bytes + 1, size - 1, 0);  // whatever else is there, no waiting
    if (more > 0) {
        count += more;
    }

    return (size_t)count;
}

bool uart_link_send(const char *text)
{
    if (uart_write_bytes(UART_PORT, text, strlen(text)) < 0) {
        ESP_LOGW(TAG, "send failed");
        return false;
    }

    return true;
}
