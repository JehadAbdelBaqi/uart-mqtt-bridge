#include <unity.h>

#include "fakes.h"
#include "message_routing.h"
#include "upstream_messaging.h"

/** Runs before each test. Starts with nothing sent or published, and the upstream messaging started. */
void setUp(void)
{
    fakes_reset();
    start_upstream_messaging();
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** A handshake message is answered to the MCU by the bridge itself. */
static void test_handshake_is_answered_to_the_mcu(void)
{
    handle_downstream_messages("H,request");

    TEST_ASSERT_EQUAL_STRING("L,H,request,ok\n", fake_uart_last_sent());
}

/** A handshake message goes no further than the bridge. */
static void test_handshake_is_not_published(void)
{
    handle_downstream_messages("H,request");

    TEST_ASSERT_EQUAL_INT(0, fake_mqtt_publish_count());
}

/** Any other message is published, as it is. */
static void test_other_message_is_published_unchanged(void)
{
    handle_downstream_messages("W,5,1791154800,2350");

    TEST_ASSERT_EQUAL_INT(1, fake_mqtt_publish_count());
    TEST_ASSERT_EQUAL_STRING("W,5,1791154800,2350", fake_mqtt_last_text());
}

/** A message that is passed on is not answered there and then: the answer waits for the broker. */
static void test_other_message_is_not_answered_at_once(void)
{
    handle_downstream_messages("W,5,1791154800,2350");

    TEST_ASSERT_EQUAL_INT(0, fake_uart_send_count());
}

/** A message from the MCU is written to the log as one going up. */
static void test_message_is_logged_as_up(void)
{
    handle_downstream_messages("W,5,2350");

    TEST_ASSERT_EQUAL_STRING("up: W,5,2350", fake_log_last());
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_handshake_is_answered_to_the_mcu);
    RUN_TEST(test_handshake_is_not_published);

    RUN_TEST(test_other_message_is_published_unchanged);
    RUN_TEST(test_other_message_is_not_answered_at_once);

    RUN_TEST(test_message_is_logged_as_up);

    return UNITY_END();
}
