#include "downstream_messaging.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "generated/config_in_use.h"
#include "helpers.h"
#include "line_framing.h"
#include "downstream_connection.h"
#include "message_routing.h"
#include "uart_link.h"

#define READ_CHUNK_SIZE 64  // most bytes taken from the UART per read

/**
 * @brief Copies the start of a message: its first few fields, as they are written in it.
 *
 * With ',' as the separator and 2 fields, "W,5" is copied from "W,5,2350".
 *
 * @param message     The message, ending in '\0'
 * @param separator   The character between the fields of a message
 * @param field_count How many fields to copy, counting the message's letter as one
 * @param start       Filled in with the start, ending in '\0'
 * @param size        The size of start
 */
static void copy_message_start(const char *message, char separator, size_t field_count, char *start, size_t size)
{
    size_t separators = 0;
    size_t length = 0;

    while (message[length] != '\0' && length < size - 1) {
        if (message[length] == separator) {
            separators++;
        }
        if (separators == field_count) {
            break;  // this separator ends the last field wanted
        }
        start[length] = message[length];
        length++;
    }
    start[length] = '\0';
}

/**
 * @brief Generates the bridge's answer to a message, as text: ANSWER_FORMAT filled in with the
 *        start of the message and the text of the status.
 *
 * @param message The message being answered, ending in '\0'
 * @param status  The status to give
 * @param answer  Filled in with the answer, ending in '\0'; LINE_MAX_LEN + 1 characters. An
 *                answer that does not fit is cut short
 * @return The length of the whole answer, which is more than LINE_MAX_LEN if it did not fit
 */
static size_t generate_downstream_answer(const char *message, status_t status, char *answer)
{
    char start[LINE_MAX_LEN + 1];

    copy_message_start(message, FIELD_SEPARATOR, ANSWER_FIELD_COUNT, start, sizeof(start));

    return (size_t)snprintf(answer, LINE_MAX_LEN + 1, ANSWER_FORMAT, start, status_text[status]);
}

void send_answer_downstream(const char *message, status_t status)
{
    char answer[LINE_MAX_LEN + 1];
    char line[DOWNSTREAM_LINE_SIZE];

    generate_downstream_answer(message, status, answer);

    // strlen: what is in the buffer, which is never more than fits in it
    format_message_for_downstream(answer, strlen(answer), line);
    uart_link_send(line);
    log_message("down", line);
}

void send_connection_status_downstream(const char *message)
{
    if (STATUS_IS_CON_ERR_WIFI) {
        send_answer_downstream(message, STATUS_CON_ERR_WIFI);
        return;
    }
    if (STATUS_IS_CON_ERR_BROKER) {
        send_answer_downstream(message, STATUS_CON_ERR_BROKER);
        return;
    }
    send_answer_downstream(message, STATUS_OK);
}

void handle_handshake(const char *message)
{
    note_mcu_connected();  // the bridge answers whatever its status: the MCU and the bridge can talk
    send_connection_status_downstream(message);
}

/**
 * @brief Adds one byte from the MCU to the message being collected, and has the message dealt
 *        with if the byte ended it.
 *
 * @param byte The byte received
 */
static void handle_downstream_byte(uint8_t byte)
{
    const char *message = line_framing_add_byte(byte);

    if (message == NULL) {
        return;  // the message is not whole yet
    }
    handle_downstream_messages(message);
}

/**
 * @brief Task that reads the bytes arriving from the MCU, for ever.
 *
 * Sleeps until bytes have arrived, then hands over each one.
 *
 * @param arg Not used
 */
static void read_downstream_task(void *arg)
{
    uint8_t bytes[READ_CHUNK_SIZE];

    while (1) {
        size_t count = uart_link_read(bytes, sizeof(bytes));

        for (size_t i = 0; i < count; i++) {
            handle_downstream_byte(bytes[i]);
        }
    }
}

void start_downstream_messaging(void)
{
    xTaskCreate(read_downstream_task, "downstream", 4096, NULL, 5, NULL);
}
