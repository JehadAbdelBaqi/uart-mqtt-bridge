#ifndef FAKE_ESP_ERR_H
#define FAKE_ESP_ERR_H

// Stands in for ESP-IDF's esp_err.h: the fakes never fail, so the check only runs the call.

#define ESP_ERROR_CHECK(call) do { (void)(call); } while (0)

#endif
