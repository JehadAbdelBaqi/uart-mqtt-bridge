#include <stddef.h>
#include <string.h>

#include <unity.h>

#include "generated/config_in_use.h"
#include "line_framing.h"

/**
 * @brief Hands a text to the framing one byte at a time.
 *
 * @param bytes The bytes, ending in '\0'
 * @return What the framing gave back for the last byte: a line, or NULL
 */
static const char *feed(const char *bytes)
{
    const char *line = NULL;

    for (size_t i = 0; bytes[i] != '\0'; i++) {
        line = line_framing_add_byte((uint8_t)bytes[i]);
    }

    return line;
}

/** Runs before each test. Ends whatever line an earlier test left unfinished. */
void setUp(void)
{
    line_framing_add_byte('\n');
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** Bytes with no line end among them are not a line yet. */
static void test_no_line_before_the_line_end(void)
{
    TEST_ASSERT_NULL(feed("W,5,2350"));
}

/** The line end hands over what was collected, without the line end itself. */
static void test_line_end_gives_the_line(void)
{
    TEST_ASSERT_EQUAL_STRING("W,5,2350", feed("W,5,2350\n"));
}

/** A skipped character is left out of the line. */
static void test_skipped_character_is_left_out(void)
{
    TEST_ASSERT_EQUAL_STRING("W,5", feed("W,5\r\n"));
}

/** A skipped character in the middle of a line is left out too. */
static void test_skipped_character_in_the_middle_is_left_out(void)
{
    TEST_ASSERT_EQUAL_STRING("W,5", feed("W,\r5\n"));
}

/** Each line starts afresh: nothing of the one before is in it. */
static void test_second_line_starts_afresh(void)
{
    feed("W,5,2350\n");

    TEST_ASSERT_EQUAL_STRING("R,7", feed("R,7\n"));
}

/** A line end straight after another gives an empty line. */
static void test_line_end_alone_gives_an_empty_line(void)
{
    TEST_ASSERT_EQUAL_STRING("", feed("\n"));
}

/** A line of exactly the limit is handed over whole. */
static void test_line_at_the_limit_is_whole(void)
{
    char bytes[LINE_MAX_LEN + 2];

    memset(bytes, 'a', LINE_MAX_LEN);
    bytes[LINE_MAX_LEN] = '\n';
    bytes[LINE_MAX_LEN + 1] = '\0';

    TEST_ASSERT_EQUAL_size_t(LINE_MAX_LEN, strlen(feed(bytes)));
}

/** Bytes past the limit are not kept: the line is handed over cut short at the limit. */
static void test_bytes_past_the_limit_are_not_kept(void)
{
    char bytes[LINE_MAX_LEN + 12];

    memset(bytes, 'a', LINE_MAX_LEN + 10);
    bytes[LINE_MAX_LEN + 10] = '\n';
    bytes[LINE_MAX_LEN + 11] = '\0';

    TEST_ASSERT_EQUAL_size_t(LINE_MAX_LEN, strlen(feed(bytes)));
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_no_line_before_the_line_end);
    RUN_TEST(test_line_end_gives_the_line);
    RUN_TEST(test_skipped_character_is_left_out);
    RUN_TEST(test_skipped_character_in_the_middle_is_left_out);
    RUN_TEST(test_second_line_starts_afresh);
    RUN_TEST(test_line_end_alone_gives_an_empty_line);

    RUN_TEST(test_line_at_the_limit_is_whole);
    RUN_TEST(test_bytes_past_the_limit_are_not_kept);

    return UNITY_END();
}
