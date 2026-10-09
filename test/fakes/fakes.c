#include "fakes.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_link.h"
#include "uart_link.h"
#include "wifi_link.h"

#define TEXT_SIZE         256  // room for any text a fake keeps
#define SUBSCRIPTION_MAX  8
#define TIMER_MAX         4

// --- What each fake has recorded, and what it answers

static char uart_last_sent[TEXT_SIZE];
static int uart_send_count;

static bool wifi_up;
static bool broker_up;

static int mqtt_next_message_id;
static int mqtt_publish_count;
static char mqtt_last_topic[TEXT_SIZE];
static char mqtt_last_text[TEXT_SIZE];
static const char *mqtt_subscriptions[SUBSCRIPTION_MAX];
static int mqtt_subscribe_count;
static int mqtt_drop_count;
static mqtt_link_handlers_t mqtt_handlers;

static int64_t time_us;

/** A timer the application has created. */
struct fake_timer {
    const char *name;
    void (*callback)(void *arg);
    bool running;
};

static struct fake_timer timers[TIMER_MAX];
static int timer_count;

static led_mcu_state_t led_mcu_state;

static char log_last[TEXT_SIZE];
static int log_count;

/**
 * @brief Copies a text into one of the fakes' own buffers, cut short if it is too long.
 *
 * @param destination Where to keep it; TEXT_SIZE characters
 * @param text        The text
 */
static void keep_text(char *destination, const char *text)
{
    snprintf(destination, TEXT_SIZE, "%s", text);
}

/**
 * @brief Finds a timer by the name it was created with.
 *
 * @param name The name
 * @return The timer, or NULL if none has that name
 */
static struct fake_timer *find_timer(const char *name)
{
    for (int i = 0; i < timer_count; i++) {
        if (strcmp(timers[i].name, name) == 0) {
            return &timers[i];
        }
    }

    return NULL;
}

void fakes_reset(void)
{
    uart_last_sent[0] = '\0';
    uart_send_count = 0;

    wifi_up = true;
    broker_up = true;

    mqtt_next_message_id = 1;
    mqtt_publish_count = 0;
    mqtt_last_topic[0] = '\0';
    mqtt_last_text[0] = '\0';
    mqtt_subscribe_count = 0;
    mqtt_drop_count = 0;

    time_us = 0;
    for (int i = 0; i < timer_count; i++) {
        timers[i].running = false;
    }

    led_mcu_state = LED_MCU_NONE;

    log_last[0] = '\0';
    log_count = 0;
}

// --- The UART link

bool uart_link_send(const char *text)
{
    keep_text(uart_last_sent, text);
    uart_send_count++;
    return true;
}

size_t uart_link_read(uint8_t *bytes, size_t size)
{
    (void)bytes;
    (void)size;
    return 0;  // no test runs the task that reads the UART
}

const char *fake_uart_last_sent(void)
{
    return uart_last_sent;
}

int fake_uart_send_count(void)
{
    return uart_send_count;
}

// --- Wi-Fi and the broker

bool check_wifi_link(void)
{
    return wifi_up;
}

bool check_mqtt_link(void)
{
    return broker_up;
}

void fake_set_wifi_up(bool up)
{
    wifi_up = up;
}

void fake_set_broker_up(bool up)
{
    broker_up = up;
}

void mqtt_link_set_handlers(const mqtt_link_handlers_t *new_handlers)
{
    mqtt_handlers = *new_handlers;
}

int mqtt_link_publish(const char *topic, const char *text)
{
    keep_text(mqtt_last_topic, topic);
    keep_text(mqtt_last_text, text);
    mqtt_publish_count++;
    return mqtt_next_message_id;
}

bool mqtt_link_subscribe(const char *topic)
{
    if (mqtt_subscribe_count < SUBSCRIPTION_MAX) {
        mqtt_subscriptions[mqtt_subscribe_count] = topic;
    }
    mqtt_subscribe_count++;
    return true;
}

void mqtt_link_drop_connection(void)
{
    mqtt_drop_count++;
}

void fake_mqtt_set_next_message_id(int message_id)
{
    mqtt_next_message_id = message_id;
}

int fake_mqtt_publish_count(void)
{
    return mqtt_publish_count;
}

const char *fake_mqtt_last_topic(void)
{
    return mqtt_last_topic;
}

const char *fake_mqtt_last_text(void)
{
    return mqtt_last_text;
}

int fake_mqtt_subscribe_count(void)
{
    return mqtt_subscribe_count;
}

const char *fake_mqtt_subscribed_topic(int index)
{
    return mqtt_subscriptions[index];
}

int fake_mqtt_drop_count(void)
{
    return mqtt_drop_count;
}

void fake_mqtt_connect(void)
{
    mqtt_handlers.connected();
}

void fake_mqtt_confirm(int message_id)
{
    mqtt_handlers.published(message_id);
}

void fake_mqtt_deliver(const char *message, size_t length)
{
    mqtt_handlers.message(message, length);
}

// --- Timers and the time

int esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *timer)
{
    struct fake_timer *existing = find_timer(args->name);

    if (existing != NULL) {
        *timer = existing;  // created again by a later test: the same timer
        return 0;
    }

    timers[timer_count].name = args->name;
    timers[timer_count].callback = args->callback;
    timers[timer_count].running = false;
    *timer = &timers[timer_count];
    timer_count++;
    return 0;
}

int esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us)
{
    (void)timeout_us;
    timer->running = true;
    return 0;
}

int esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us)
{
    (void)period_us;
    timer->running = true;
    return 0;
}

int esp_timer_stop(esp_timer_handle_t timer)
{
    timer->running = false;
    return 0;
}

int64_t esp_timer_get_time(void)
{
    return time_us;
}

void fake_set_time_ms(uint32_t ms)
{
    time_us = (int64_t)ms * 1000;
}

bool fake_timer_is_running(const char *name)
{
    return find_timer(name)->running;
}

void fake_timer_fire(const char *name)
{
    find_timer(name)->callback(NULL);
}

// --- Tasks

int xTaskCreate(void (*task)(void *arg), const char *name, unsigned stack_size, void *arg, int priority, void *handle)
{
    (void)task;
    (void)name;
    (void)stack_size;
    (void)arg;
    (void)priority;
    (void)handle;
    return 1;
}

// --- The LED and the log

void led_show_mcu(led_mcu_state_t state)
{
    led_mcu_state = state;
}

led_mcu_state_t fake_led_mcu_state(void)
{
    return led_mcu_state;
}

void fake_log(const char *tag, const char *format, ...)
{
    va_list arguments;

    (void)tag;
    va_start(arguments, format);
    vsnprintf(log_last, sizeof(log_last), format, arguments);
    va_end(arguments);
    log_count++;
}

const char *fake_log_last(void)
{
    return log_last;
}

int fake_log_count(void)
{
    return log_count;
}
