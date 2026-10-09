#include <string.h>

#include <unity.h>

#include "fakes.h"
#include "generated/config_in_use.h"
#include "helpers.h"

// The line a message is made ready in
static char line[DOWNSTREAM_LINE_SIZE];

/** Runs before each test. Starts with nothing recorded and an empty line. */
void setUp(void)
{
    fakes_reset();
    memset(line, 0, sizeof(line));
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** An empty message is not valid. */
static void test_empty_message_is_not_valid(void)
{
    TEST_ASSERT_FALSE(message_is_valid(0));
}

/** A message of one character is valid. */
static void test_shortest_message_is_valid(void)
{
    TEST_ASSERT_TRUE(message_is_valid(1));
}

/** A message of exactly the line limit is valid. */
static void test_message_at_the_limit_is_valid(void)
{
    TEST_ASSERT_TRUE(message_is_valid(LINE_MAX_LEN));
}

/** A message one character over the line limit is not valid. */
static void test_message_over_the_limit_is_not_valid(void)
{
    TEST_ASSERT_FALSE(message_is_valid(LINE_MAX_LEN + 1));
}

/** A message made ready to send is the message with the line end after it. */
static void test_format_adds_the_line_end(void)
{
    format_message_for_downstream("C,7,read", 8, line);

    TEST_ASSERT_EQUAL_STRING("C,7,read\n", line);
}

/** Only as many bytes as the length says are taken: what follows the message is left out. */
static void test_format_takes_only_the_length_given(void)
{
    format_message_for_downstream("C,7,readXXXX", 8, line);

    TEST_ASSERT_EQUAL_STRING("C,7,read\n", line);
}

/** A message is written to the log with its direction in front. */
static void test_log_has_the_direction_and_the_message(void)
{
    log_message("up", "W,5,2350");

    TEST_ASSERT_EQUAL_STRING("up: W,5,2350", fake_log_last());
}

/** A line end in the message ends what is logged. */
static void test_log_stops_at_the_line_end(void)
{
    log_message("down", "L,W,5,ok\n");

    TEST_ASSERT_EQUAL_STRING("down: L,W,5,ok", fake_log_last());
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_empty_message_is_not_valid);
    RUN_TEST(test_shortest_message_is_valid);
    RUN_TEST(test_message_at_the_limit_is_valid);
    RUN_TEST(test_message_over_the_limit_is_not_valid);

    RUN_TEST(test_format_adds_the_line_end);
    RUN_TEST(test_format_takes_only_the_length_given);

    RUN_TEST(test_log_has_the_direction_and_the_message);
    RUN_TEST(test_log_stops_at_the_line_end);

    return UNITY_END();
}
