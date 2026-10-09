#include <unity.h>

#include "fakes.h"
#include "status.h"

/** Runs before each test. Starts with Wi-Fi up and the broker connected. */
void setUp(void)
{
    fakes_reset();
}

/** Runs after each test. Nothing to clear up. */
void tearDown(void)
{
}

/** With Wi-Fi up and the broker connected, the status is ok and neither of the other two. */
static void test_both_up_is_ok(void)
{
    TEST_ASSERT_TRUE(STATUS_IS_OK);
    TEST_ASSERT_FALSE(STATUS_IS_CON_ERR_WIFI);
    TEST_ASSERT_FALSE(STATUS_IS_CON_ERR_BROKER);
}

/** With Wi-Fi up and the broker not connected, the status is the broker one alone. */
static void test_broker_down_is_the_broker_status(void)
{
    fake_set_broker_up(false);

    TEST_ASSERT_FALSE(STATUS_IS_OK);
    TEST_ASSERT_FALSE(STATUS_IS_CON_ERR_WIFI);
    TEST_ASSERT_TRUE(STATUS_IS_CON_ERR_BROKER);
}

/** With Wi-Fi down, the status is the Wi-Fi one alone. */
static void test_wifi_down_is_the_wifi_status(void)
{
    fake_set_wifi_up(false);
    fake_set_broker_up(false);

    TEST_ASSERT_FALSE(STATUS_IS_OK);
    TEST_ASSERT_TRUE(STATUS_IS_CON_ERR_WIFI);
    TEST_ASSERT_FALSE(STATUS_IS_CON_ERR_BROKER);
}

/** With Wi-Fi down, the status is the Wi-Fi one whatever the broker's flag says. */
static void test_wifi_down_comes_before_the_broker(void)
{
    fake_set_wifi_up(false);
    fake_set_broker_up(true);

    TEST_ASSERT_FALSE(STATUS_IS_OK);
    TEST_ASSERT_TRUE(STATUS_IS_CON_ERR_WIFI);
    TEST_ASSERT_FALSE(STATUS_IS_CON_ERR_BROKER);
}

/** Each status has the text the config gives it. */
static void test_each_status_has_its_text_from_the_config(void)
{
    TEST_ASSERT_EQUAL_STRING(STATUS_TEXT_OK, status_text[STATUS_OK]);
    TEST_ASSERT_EQUAL_STRING(STATUS_TEXT_CON_ERR_WIFI, status_text[STATUS_CON_ERR_WIFI]);
    TEST_ASSERT_EQUAL_STRING(STATUS_TEXT_CON_ERR_BROKER, status_text[STATUS_CON_ERR_BROKER]);
    TEST_ASSERT_EQUAL_STRING(STATUS_TEXT_ERR_BROKER, status_text[STATUS_ERR_BROKER]);
    TEST_ASSERT_EQUAL_STRING(STATUS_TEXT_ANS_ERR_LONG, status_text[STATUS_ANS_ERR_LONG]);
}

/** Runs every test in this file. */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_both_up_is_ok);
    RUN_TEST(test_broker_down_is_the_broker_status);
    RUN_TEST(test_wifi_down_is_the_wifi_status);
    RUN_TEST(test_wifi_down_comes_before_the_broker);

    RUN_TEST(test_each_status_has_its_text_from_the_config);

    return UNITY_END();
}
