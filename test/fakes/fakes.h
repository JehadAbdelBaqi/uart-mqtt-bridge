#ifndef FAKES_H
#define FAKES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "led.h"

// Stand-ins for everything the application calls that only exists on the board: the UART link,
// the Wi-Fi link, the MQTT link, the LED, the timers and the log. Each one records what it was
// asked to do, and a test reads that back or sets what it answers.

/**
 * @brief Puts every fake back to its starting state: nothing recorded, Wi-Fi up, the broker
 *        connected, the time at 0, no timer running.
 *
 * Call in setUp().
 */
void fakes_reset(void);

// --- The UART link to the MCU

/** @return The last text sent to the MCU, or "" if none was */
const char *fake_uart_last_sent(void);

/** @return How many texts have been sent to the MCU */
int fake_uart_send_count(void);

// --- Wi-Fi and the broker

/** @param up Whether check_wifi_link() says Wi-Fi is up */
void fake_set_wifi_up(bool up);

/** @param up Whether check_mqtt_link() says the broker is connected */
void fake_set_broker_up(bool up);

/** @param message_id What the next mqtt_link_publish() gives back; a negative number is a failure */
void fake_mqtt_set_next_message_id(int message_id);

/** @return How many messages have been published */
int fake_mqtt_publish_count(void);

/** @return The topic of the last message published, or "" if none was */
const char *fake_mqtt_last_topic(void);

/** @return The text of the last message published, or "" if none was */
const char *fake_mqtt_last_text(void);

/** @return How many topics have been subscribed to */
int fake_mqtt_subscribe_count(void);

/**
 * @param index Which subscription, from 0
 * @return The topic of that subscription
 */
const char *fake_mqtt_subscribed_topic(int index);

/** @return How many times the connection to the broker has been dropped */
int fake_mqtt_drop_count(void);

/** @brief Does what the MQTT link does when the connection to the broker is made. */
void fake_mqtt_connect(void);

/**
 * @brief Does what the MQTT link does when the broker confirms a message.
 *
 * @param message_id The ID of the message confirmed
 */
void fake_mqtt_confirm(int message_id);

/**
 * @brief Does what the MQTT link does when a message arrives from the broker.
 *
 * @param message The message's bytes
 * @param length  How many bytes it has
 */
void fake_mqtt_deliver(const char *message, size_t length);

// --- Timers and the time

/** @param ms The time since the bridge started, in milliseconds */
void fake_set_time_ms(uint32_t ms);

/**
 * @param name The name the timer was created with
 * @return true if the timer has been started and not stopped
 */
bool fake_timer_is_running(const char *name);

/**
 * @brief Makes a timer run out: calls the function it was created with.
 *
 * @param name The name the timer was created with
 */
void fake_timer_fire(const char *name);

// --- The LED and the log

/** @return The state of the connection to the MCU the LED was last told to show */
led_mcu_state_t fake_led_mcu_state(void);

/** @return The last message written to the log, or "" if none was */
const char *fake_log_last(void);

/** @return How many messages have been written to the log */
int fake_log_count(void);

#endif
