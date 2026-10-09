#include <string.h>

#include <unity.h>

#include "fakes.h"
#include "generated/config_in_use.h"
#include "upstream_messaging.h"

// The name the bridge gives the timer for the broker's confirmation
#define CONFIRM_TIMER "broker_confirm"

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

/** A message goes to the topic the config gives its first letter. */
static void test_message_goes_to_the_topic_of_its_letter(void)
{
    send_message_upstream("W,5,2350");

    TEST_ASSERT_EQUAL_STRING("test/readings", fake_mqtt_last_topic());
}

/** Another letter goes to its own topic. */
static void test_another_letter_goes_to_its_own_topic(void)
{
    send_message_upstream("R,7,ok");

    TEST_ASSERT_EQUAL_STRING("test/replies", fake_mqtt_last_topic());
}

/** The message is published as it is. */
static void test_message_is_published_unchanged(void)
{
    send_message_upstream("W,5,1791154800,2350");

    TEST_ASSERT_EQUAL_STRING("W,5,1791154800,2350", fake_mqtt_last_text());
}

/** An empty message is not published. */
static void test_empty_message_is_not_published(void)
{
    send_message_upstream("");

    TEST_ASSERT_EQUAL_INT(0, fake_mqtt_publish_count());
}

/** A message whose letter has no topic is not published. */
static void test_message_with_no_topic_is_not_published(void)
{
    send_message_upstream("X,1");

    TEST_ASSERT_EQUAL_INT(0, fake_mqtt_publish_count());
}

/** While the broker is not connected, a message is not published. */
static void test_message_is_not_published_while_the_broker_is_down(void)
{
    fake_set_broker_up(false);

    send_message_upstream("W,5,2350");

    TEST_ASSERT_EQUAL_INT(0, fake_mqtt_publish_count());
}

/** While the broker is not connected, the MCU is answered at once with the broker status. */
static void test_message_is_answered_with_the_broker_status_while_it_is_down(void)
{
    fake_set_broker_up(false);

    send_message_upstream("W,5,2350");

    TEST_ASSERT_EQUAL_STRING("L,W,5,con_err_br\n", fake_uart_last_sent());
}

/** While Wi-Fi is down, the MCU is answered at once with the Wi-Fi status. */
static void test_message_is_answered_with_the_wifi_status_while_it_is_down(void)
{
    fake_set_wifi_up(false);
    fake_set_broker_up(false);

    send_message_upstream("W,5,2350");

    TEST_ASSERT_EQUAL_STRING("L,W,5,con_err_w\n", fake_uart_last_sent());
}

/** A message that was not published starts no timer: nothing is waiting for the broker. */
static void test_message_not_published_starts_no_timer(void)
{
    fake_set_broker_up(false);

    send_message_upstream("W,5,2350");

    TEST_ASSERT_FALSE(fake_timer_is_running(CONFIRM_TIMER));
}

/** Sending a message starts the timer for the broker's confirmation. */
static void test_sending_starts_the_confirmation_timer(void)
{
    send_message_upstream("W,5,2350");

    TEST_ASSERT_TRUE(fake_timer_is_running(CONFIRM_TIMER));
}

/** A message the MQTT link could not publish starts no timer. */
static void test_failed_publish_starts_no_timer(void)
{
    fake_mqtt_set_next_message_id(-1);

    send_message_upstream("W,5,2350");

    TEST_ASSERT_FALSE(fake_timer_is_running(CONFIRM_TIMER));
}

/** When the broker confirms the message, the MCU is answered ok for it. */
static void test_confirmation_answers_ok(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");

    fake_mqtt_confirm(41);

    TEST_ASSERT_EQUAL_STRING("L,W,5,ok\n", fake_uart_last_sent());
}

/** The confirmation stops the timer. */
static void test_confirmation_stops_the_timer(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");

    fake_mqtt_confirm(41);

    TEST_ASSERT_FALSE(fake_timer_is_running(CONFIRM_TIMER));
}

/** A confirmation for a different message answers nothing, and the timer keeps running. */
static void test_confirmation_of_another_message_changes_nothing(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");

    fake_mqtt_confirm(99);

    TEST_ASSERT_EQUAL_INT(0, fake_uart_send_count());
    TEST_ASSERT_TRUE(fake_timer_is_running(CONFIRM_TIMER));
}

/** A message is answered once: a second confirmation for it answers nothing more. */
static void test_second_confirmation_answers_nothing_more(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");
    fake_mqtt_confirm(41);

    fake_mqtt_confirm(41);

    TEST_ASSERT_EQUAL_INT(1, fake_uart_send_count());
}

/** When the timer runs out first, the MCU is answered that the broker has a fault. */
static void test_timeout_answers_the_broker_fault(void)
{
    send_message_upstream("W,6,2351");

    fake_timer_fire(CONFIRM_TIMER);

    TEST_ASSERT_EQUAL_STRING("L,W,6,err_br\n", fake_uart_last_sent());
}

/** When the timer runs out, the connection to the broker is dropped, to be made afresh. */
static void test_timeout_drops_the_broker_connection(void)
{
    send_message_upstream("W,6,2351");

    fake_timer_fire(CONFIRM_TIMER);

    TEST_ASSERT_EQUAL_INT(1, fake_mqtt_drop_count());
}

/** A confirmation that comes after the timer has run out answers nothing more. */
static void test_confirmation_after_the_timeout_answers_nothing_more(void)
{
    fake_mqtt_set_next_message_id(42);
    send_message_upstream("W,6,2351");
    fake_timer_fire(CONFIRM_TIMER);

    fake_mqtt_confirm(42);

    TEST_ASSERT_EQUAL_INT(1, fake_uart_send_count());
}

/** The timer running out after the message was confirmed does nothing. */
static void test_timeout_after_the_confirmation_does_nothing(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");
    fake_mqtt_confirm(41);

    fake_timer_fire(CONFIRM_TIMER);

    TEST_ASSERT_EQUAL_INT(1, fake_uart_send_count());
    TEST_ASSERT_EQUAL_INT(0, fake_mqtt_drop_count());
}

/** A message sent while another is held takes its place: the answer is for the newer one. */
static void test_newer_message_replaces_the_held_one(void)
{
    fake_mqtt_set_next_message_id(41);
    send_message_upstream("W,5,2350");
    fake_mqtt_set_next_message_id(42);
    send_message_upstream("W,6,2351");

    fake_mqtt_confirm(42);

    TEST_ASSERT_EQUAL_STRING("L,W,6,ok\n", fake_uart_last_sent());
}

/** When the connection to the broker is made, every topic in the config's list is subscribed to. */
static void test_connecting_subscribes_to_every_down_topic(void)
{
    fake_mqtt_connect();

    TEST_ASSERT_EQUAL_INT(2, fake_mqtt_subscribe_count());
    TEST_ASSERT_EQUAL_STRING("test/commands", fake_mqtt_subscribed_topic(0));
    TEST_ASSERT_EQUAL_STRING("test/acks", fake_mqtt_subscribed_topic(1));
}

/** A message from the broker is sent to the MCU as it is, with the line end after it. */
static void test_broker_message_is_sent_down_with_the_line_end(void)
{
    fake_mqtt_deliver("C,7,read", 8);

    TEST_ASSERT_EQUAL_STRING("C,7,read\n", fake_uart_last_sent());
}

/** Only as many bytes as the broker's message has are sent: what follows them is left out. */
static void test_broker_message_takes_only_its_length(void)
{
    fake_mqtt_deliver("C,7,readXXXX", 8);

    TEST_ASSERT_EQUAL_STRING("C,7,read\n", fake_uart_last_sent());
}

/** An empty message from the broker is not sent to the MCU. */
static void test_empty_broker_message_is_not_sent_down(void)
{
    fake_mqtt_deliver("", 0);

    TEST_ASSERT_EQUAL_INT(0, fake_uart_send_count());
}

/** A message from the broker longer than the line limit is not sent to the MCU. */
static void test_broker_message_over_the_limit_is_not_sent_down(void)
{
    char message[LINE_MAX_LEN + 1];

    memset(message, 'a', sizeof(message));

    fake_mqtt_deliver(message, sizeof(message));

    TEST_ASSERT_EQUAL_INT(0, fake_uart_send_count());
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_message_goes_to_the_topic_of_its_letter);
    RUN_TEST(test_another_letter_goes_to_its_own_topic);
    RUN_TEST(test_message_is_published_unchanged);
    RUN_TEST(test_empty_message_is_not_published);
    RUN_TEST(test_message_with_no_topic_is_not_published);

    RUN_TEST(test_message_is_not_published_while_the_broker_is_down);
    RUN_TEST(test_message_is_answered_with_the_broker_status_while_it_is_down);
    RUN_TEST(test_message_is_answered_with_the_wifi_status_while_it_is_down);
    RUN_TEST(test_message_not_published_starts_no_timer);

    RUN_TEST(test_sending_starts_the_confirmation_timer);
    RUN_TEST(test_failed_publish_starts_no_timer);

    RUN_TEST(test_confirmation_answers_ok);
    RUN_TEST(test_confirmation_stops_the_timer);
    RUN_TEST(test_confirmation_of_another_message_changes_nothing);
    RUN_TEST(test_second_confirmation_answers_nothing_more);

    RUN_TEST(test_timeout_answers_the_broker_fault);
    RUN_TEST(test_timeout_drops_the_broker_connection);
    RUN_TEST(test_confirmation_after_the_timeout_answers_nothing_more);
    RUN_TEST(test_timeout_after_the_confirmation_does_nothing);
    RUN_TEST(test_newer_message_replaces_the_held_one);

    RUN_TEST(test_connecting_subscribes_to_every_down_topic);
    RUN_TEST(test_broker_message_is_sent_down_with_the_line_end);
    RUN_TEST(test_broker_message_takes_only_its_length);
    RUN_TEST(test_empty_broker_message_is_not_sent_down);
    RUN_TEST(test_broker_message_over_the_limit_is_not_sent_down);

    return UNITY_END();
}
