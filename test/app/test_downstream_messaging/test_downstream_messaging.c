#include <string.h>

#include <unity.h>

#include "downstream_messaging.h"
#include "fakes.h"
#include "generated/config_in_use.h"

/** Runs before each test. Starts with nothing sent, Wi-Fi up and the broker connected. */
void setUp(void)
{
    fakes_reset();
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** The answer is the config's layout, with the message's letter and first field and the status. */
static void test_answer_has_the_start_of_the_message_and_the_status(void)
{
    send_answer_downstream("W,5,1791154800,2350", STATUS_OK);

    TEST_ASSERT_EQUAL_STRING("L,W,5,ok\n", fake_uart_last_sent());
}

/** A message with nothing after its first field is answered with all of it. */
static void test_answer_to_a_message_of_two_fields(void)
{
    send_answer_downstream("H,request", STATUS_OK);

    TEST_ASSERT_EQUAL_STRING("L,H,request,ok\n", fake_uart_last_sent());
}

/** The answer carries the text of whichever status it is given. */
static void test_answer_carries_the_status_given(void)
{
    send_answer_downstream("W,5,2350", STATUS_ERR_BROKER);

    TEST_ASSERT_EQUAL_STRING("L,W,5,err_br\n", fake_uart_last_sent());
}

/** One answer is one send to the MCU. */
static void test_answer_is_sent_once(void)
{
    send_answer_downstream("W,5,2350", STATUS_OK);

    TEST_ASSERT_EQUAL_INT(1, fake_uart_send_count());
}

/** An answer that would be longer than the line limit goes out cut short at the limit. */
static void test_answer_too_long_is_cut_at_the_limit(void)
{
    char message[LINE_MAX_LEN + 1];

    memset(message, 'a', LINE_MAX_LEN);
    message[0] = 'R';
    message[1] = FIELD_SEPARATOR;
    message[LINE_MAX_LEN] = '\0';

    send_answer_downstream(message, STATUS_OK);

    TEST_ASSERT_EQUAL_size_t(LINE_MAX_LEN + 1, strlen(fake_uart_last_sent()));  // the limit, and the line end
}

/** A handshake is answered ok while Wi-Fi is up and the broker is connected. */
static void test_handshake_answers_ok_when_all_is_up(void)
{
    handle_handshake("H,request");

    TEST_ASSERT_EQUAL_STRING("L,H,request,ok\n", fake_uart_last_sent());
}

/** A handshake is answered with the broker status while Wi-Fi is up and the broker is not connected. */
static void test_handshake_answers_the_broker_status(void)
{
    fake_set_broker_up(false);

    handle_handshake("H,request");

    TEST_ASSERT_EQUAL_STRING("L,H,request,con_err_br\n", fake_uart_last_sent());
}

/** A handshake is answered with the Wi-Fi status while Wi-Fi is down. */
static void test_handshake_answers_the_wifi_status(void)
{
    fake_set_wifi_up(false);
    fake_set_broker_up(false);

    handle_handshake("H,request");

    TEST_ASSERT_EQUAL_STRING("L,H,request,con_err_w\n", fake_uart_last_sent());
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_answer_has_the_start_of_the_message_and_the_status);
    RUN_TEST(test_answer_to_a_message_of_two_fields);
    RUN_TEST(test_answer_carries_the_status_given);
    RUN_TEST(test_answer_is_sent_once);
    RUN_TEST(test_answer_too_long_is_cut_at_the_limit);

    RUN_TEST(test_handshake_answers_ok_when_all_is_up);
    RUN_TEST(test_handshake_answers_the_broker_status);
    RUN_TEST(test_handshake_answers_the_wifi_status);

    return UNITY_END();
}
