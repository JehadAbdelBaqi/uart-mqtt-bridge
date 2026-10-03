#include "dummy_source.h"

#include <inttypes.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "uart_link.h"

// Off unless the config switches it on
#ifndef DUMMY_LINE_INTERVAL_MS
#define DUMMY_LINE_INTERVAL_MS 0
#endif

static const char *TAG = "dummy";

/**
 * @brief Task that writes a numbered line to the UART at the set interval.
 *
 * @param arg Not used
 */
static void dummy_task(void *arg)
{
    char line[LINE_MAX_LEN + 2];  // the line, then '\n' and '\0'
    uint32_t sequence = 0;

    while (1) {
        snprintf(line, sizeof(line), "T,%" PRIu32 ",%" PRIu32 "\n", sequence, esp_log_timestamp());
        uart_link_send(line);
        sequence++;

        vTaskDelay(pdMS_TO_TICKS(DUMMY_LINE_INTERVAL_MS));
    }
}

void dummy_source_init(void)
{
    if (DUMMY_LINE_INTERVAL_MS == 0) {
        return;
    }

    ESP_LOGW(TAG, "dummy data source on: a line every %d ms", DUMMY_LINE_INTERVAL_MS);
    xTaskCreate(dummy_task, "dummy", 4096, NULL, 5, NULL);
}
