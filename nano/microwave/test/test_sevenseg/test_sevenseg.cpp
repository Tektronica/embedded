#include <unity.h>

#include "SevenSegment.h"

using namespace sevenseg;

void setUp() {}
void tearDown() {}

void test_seconds_to_digits_endpoints() {
  Digits d0 = secondsToDigits(0);
  TEST_ASSERT_EQUAL_UINT8(0, d0.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(0, d0.minutesOnes);
  TEST_ASSERT_EQUAL_UINT8(0, d0.secondsTens);
  TEST_ASSERT_EQUAL_UINT8(0, d0.secondsOnes);

  Digits dMax = secondsToDigits(MAX_SECONDS);  // 99:59
  TEST_ASSERT_EQUAL_UINT8(9, dMax.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(9, dMax.minutesOnes);
  TEST_ASSERT_EQUAL_UINT8(5, dMax.secondsTens);
  TEST_ASSERT_EQUAL_UINT8(9, dMax.secondsOnes);
}

void test_seconds_to_digits_mid_value() {
  Digits d = secondsToDigits(125);  // 2:05
  TEST_ASSERT_EQUAL_UINT8(0, d.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(2, d.minutesOnes);
  TEST_ASSERT_EQUAL_UINT8(0, d.secondsTens);
  TEST_ASSERT_EQUAL_UINT8(5, d.secondsOnes);
}

void test_seconds_to_digits_clamps_above_max() {
  Digits d = secondsToDigits(60000);
  Digits dMax = secondsToDigits(MAX_SECONDS);
  TEST_ASSERT_EQUAL_UINT8(dMax.minutesTens, d.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(dMax.secondsOnes, d.secondsOnes);
}

void test_blink_on_splits_period_in_half() {
  TEST_ASSERT_TRUE(blinkOn(0, 10));
  TEST_ASSERT_TRUE(blinkOn(4, 10));
  TEST_ASSERT_FALSE(blinkOn(5, 10));
  TEST_ASSERT_FALSE(blinkOn(9, 10));
  TEST_ASSERT_TRUE(blinkOn(10, 10));  // wraps to the next cycle
}

void test_blink_on_zero_period_is_always_on() {
  TEST_ASSERT_TRUE(blinkOn(0, 0));
  TEST_ASSERT_TRUE(blinkOn(123, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_seconds_to_digits_endpoints);
  RUN_TEST(test_seconds_to_digits_mid_value);
  RUN_TEST(test_seconds_to_digits_clamps_above_max);
  RUN_TEST(test_blink_on_splits_period_in_half);
  RUN_TEST(test_blink_on_zero_period_is_always_on);
  return UNITY_END();
}
