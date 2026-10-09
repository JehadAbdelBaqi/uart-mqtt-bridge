#ifndef FAKE_ESP_LOG_H
#define FAKE_ESP_LOG_H

// Stands in for ESP-IDF's esp_log.h: every log message is kept by the fakes, not printed.

void fake_log(const char *tag, const char *format, ...) __attribute__((format(printf, 2, 3)));

#define ESP_LOGI(tag, ...) fake_log(tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) fake_log(tag, __VA_ARGS__)
#define ESP_LOGE(tag, ...) fake_log(tag, __VA_ARGS__)

#endif
