#include "helpers.h"

#include <string.h>

#include "esp_log.h"

#include "generated/config_in_use.h"

// Off unless the config switches it on
#ifndef LOG_LINES
#define LOG_LINES 0
#endif

static const char *TAG = "message";

bool message_is_valid(size_t length)
{
    if (length == 0) {
        return false;  // empty message: nothing to pass on
    }
    if (length > LINE_MAX_LEN) {
        ESP_LOGW(TAG, "message not valid: longer than %d characters", LINE_MAX_LEN);
        return false;
    }
    return true;
}

void format_message_for_downstream(const char *message, size_t length, char *line)
{
    memcpy(line, message, length);
    line[length] = LINE_END_CHARACTERS[0];
    line[length + 1] = '\0';
}

void log_message(const char *direction, const char *message)
{
    if (!LOG_LINES) {
        return;
    }

    // Up to the line end, if the message has one, so the log has no empty lines in it
    ESP_LOGI(TAG, "%s: %.*s", direction, (int)strcspn(message, LINE_END_CHARACTERS), message);
}
