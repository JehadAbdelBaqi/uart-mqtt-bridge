#include <stdint.h>

#include <unity.h>

#include "downstream_connection.h"
#include "fakes.h"
#include "generated/config_in_use.h"

// The name the bridge gives the timer that watches for the MCU
#define WATCH_TIMER "mcu_watch"

// The time in these tests, in milliseconds. It only goes forwards, from one test to the next, as
// the bridge's own time does: the code keeps the time the MCU was last heard between tests.
static uint32_t clock_ms = 0;

/**
 * @brief Moves the time on, and has the bridge look at the MCU's silence as its timer would.
 *
 * @param ms How far to move the time on, in milliseconds
 */
static void wait_ms(uint32_t ms)
{
    clock_ms += ms;
    fake_set_time_ms(clock_ms);
    fake_timer_fire(WATCH_TIMER);
}

/** Runs before each test. Starts with the watch running and the MCU long gone. */
void setUp(void)
{
    fakes_reset();
    start_mcu_connection_watch();
    wait_ms(3 * MCU_QUIET_LIMIT_MS);
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** Until a handshake is answered, the LED shows the bridge waiting for the MCU. */
static void test_waiting_until_a_handshake_is_answered(void)
{
    TEST_ASSERT_EQUAL_INT(LED_MCU_WAITING, fake_led_mcu_state());
}

/** An answered handshake makes the connection. */
static void test_answered_handshake_makes_the_connection(void)
{
    note_mcu_connected();
    wait_ms(0);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CONNECTED, fake_led_mcu_state());
}

/** A message alone does not make the connection: only a handshake does. */
static void test_message_alone_does_not_make_the_connection(void)
{
    note_mcu_heard();
    wait_ms(0);

    TEST_ASSERT_EQUAL_INT(LED_MCU_WAITING, fake_led_mcu_state());
}

/** The connection holds while the MCU has been silent for less than the quiet limit. */
static void test_connected_just_under_the_quiet_limit(void)
{
    note_mcu_connected();

    wait_ms(MCU_QUIET_LIMIT_MS - 1);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CONNECTED, fake_led_mcu_state());
}

/** Once the MCU has been silent for the quiet limit, the LED shows it as quiet. */
static void test_quiet_at_the_quiet_limit(void)
{
    note_mcu_connected();

    wait_ms(MCU_QUIET_LIMIT_MS);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CHECKING, fake_led_mcu_state());
}

/** A message from an MCU shown as quiet puts the LED back to connected. */
static void test_message_from_a_quiet_mcu_brings_it_back(void)
{
    note_mcu_connected();
    wait_ms(MCU_QUIET_LIMIT_MS);

    note_mcu_heard();
    wait_ms(0);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CONNECTED, fake_led_mcu_state());
}

/** Each message starts the silence again: the limit counts from the last one heard. */
static void test_each_message_starts_the_silence_again(void)
{
    note_mcu_connected();
    wait_ms(MCU_QUIET_LIMIT_MS - 1);
    note_mcu_heard();

    wait_ms(MCU_QUIET_LIMIT_MS - 1);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CONNECTED, fake_led_mcu_state());
}

/** Still only quiet just under twice the quiet limit. */
static void test_quiet_just_under_twice_the_quiet_limit(void)
{
    note_mcu_connected();

    wait_ms(2 * MCU_QUIET_LIMIT_MS - 1);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CHECKING, fake_led_mcu_state());
}

/** Once the MCU has been silent for twice the quiet limit, it counts as gone. */
static void test_gone_at_twice_the_quiet_limit(void)
{
    note_mcu_connected();

    wait_ms(2 * MCU_QUIET_LIMIT_MS);

    TEST_ASSERT_EQUAL_INT(LED_MCU_WAITING, fake_led_mcu_state());
}

/** An MCU that has gone is not brought back by a message: it has to shake hands again. */
static void test_gone_mcu_is_not_brought_back_by_a_message(void)
{
    note_mcu_connected();
    wait_ms(2 * MCU_QUIET_LIMIT_MS);

    note_mcu_heard();
    wait_ms(0);

    TEST_ASSERT_EQUAL_INT(LED_MCU_WAITING, fake_led_mcu_state());
}

/** An MCU that has gone is brought back by a handshake. */
static void test_gone_mcu_is_brought_back_by_a_handshake(void)
{
    note_mcu_connected();
    wait_ms(2 * MCU_QUIET_LIMIT_MS);

    note_mcu_connected();
    wait_ms(0);

    TEST_ASSERT_EQUAL_INT(LED_MCU_CONNECTED, fake_led_mcu_state());
}

/** Starting the watch starts its timer. */
static void test_starting_the_watch_starts_its_timer(void)
{
    TEST_ASSERT_TRUE(fake_timer_is_running(WATCH_TIMER));
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_starting_the_watch_starts_its_timer);
    RUN_TEST(test_waiting_until_a_handshake_is_answered);
    RUN_TEST(test_answered_handshake_makes_the_connection);
    RUN_TEST(test_message_alone_does_not_make_the_connection);

    RUN_TEST(test_connected_just_under_the_quiet_limit);
    RUN_TEST(test_quiet_at_the_quiet_limit);
    RUN_TEST(test_message_from_a_quiet_mcu_brings_it_back);
    RUN_TEST(test_each_message_starts_the_silence_again);

    RUN_TEST(test_quiet_just_under_twice_the_quiet_limit);
    RUN_TEST(test_gone_at_twice_the_quiet_limit);
    RUN_TEST(test_gone_mcu_is_not_brought_back_by_a_message);
    RUN_TEST(test_gone_mcu_is_brought_back_by_a_handshake);

    return UNITY_END();
}
