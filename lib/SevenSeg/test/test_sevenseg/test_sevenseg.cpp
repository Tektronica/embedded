#include <unity.h>

#include "SevenSeg.h"

using namespace sevenseg;

void setUp() {}
void tearDown() {}

// --- Font ---

void test_digits_use_standard_segment_bytes() {
  const uint8_t expected[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  for (uint8_t d = 0; d < 10; ++d) {
    TEST_ASSERT_EQUAL_HEX8(expected[d], encodeDigit(d));
    TEST_ASSERT_EQUAL_HEX8(expected[d], encodeChar(static_cast<char>('0' + d)));
  }
}

void test_digit_out_of_range_is_blank() {
  TEST_ASSERT_EQUAL_HEX8(BLANK, encodeDigit(10));
  TEST_ASSERT_EQUAL_HEX8(BLANK, encodeDigit(255));
}

void test_known_letters() {
  TEST_ASSERT_EQUAL_HEX8(SEGBIT_A | SEGBIT_D | SEGBIT_E | SEGBIT_F, encodeChar('C'));
  TEST_ASSERT_EQUAL_HEX8(0x79, encodeChar('E'));
  TEST_ASSERT_EQUAL_HEX8(0x54, encodeChar('n'));
  TEST_ASSERT_EQUAL_HEX8(0x5E, encodeChar('d'));
}

void test_case_sensitive_where_shapes_differ() {
  TEST_ASSERT_NOT_EQUAL(encodeChar('H'), encodeChar('h'));
  TEST_ASSERT_NOT_EQUAL(encodeChar('U'), encodeChar('u'));
}

void test_space_is_blank() { TEST_ASSERT_EQUAL_HEX8(BLANK, encodeChar(' ')); }

void test_illegible_or_unknown_is_dash() {
  TEST_ASSERT_EQUAL_HEX8(DASH, encodeChar('M'));
  TEST_ASSERT_EQUAL_HEX8(DASH, encodeChar('K'));
  TEST_ASSERT_EQUAL_HEX8(DASH, encodeChar('!'));
}

// --- Text ---

void test_text_right_aligned_end() {
  uint8_t segments[4];
  encodeText(" End", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(BLANK, segments[0]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('E'), segments[1]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('n'), segments[2]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('d'), segments[3]);
}

void test_text_shorter_than_count_pads_blank() {
  uint8_t segments[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  encodeText("Hi", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('H'), segments[0]);
  TEST_ASSERT_EQUAL_HEX8(BLANK, segments[2]);
  TEST_ASSERT_EQUAL_HEX8(BLANK, segments[3]);
}

void test_text_longer_than_count_truncates() {
  uint8_t segments[4];
  encodeText("HELLO", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('H'), segments[0]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('E'), segments[1]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('L'), segments[2]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('L'), segments[3]);
}

// --- Blink ---

void test_blink_on_splits_period_in_half() {
  TEST_ASSERT_TRUE(blinkOn(0, 10));
  TEST_ASSERT_TRUE(blinkOn(4, 10));
  TEST_ASSERT_FALSE(blinkOn(5, 10));
  TEST_ASSERT_FALSE(blinkOn(9, 10));
  TEST_ASSERT_TRUE(blinkOn(10, 10));
}

void test_blink_on_zero_period_is_always_on() {
  TEST_ASSERT_TRUE(blinkOn(0, 0));
  TEST_ASSERT_TRUE(blinkOn(123, 0));
}

// --- MM:SS ---

void test_seconds_to_digits_endpoints() {
  Digits d0 = secondsToDigits(0);
  TEST_ASSERT_EQUAL_UINT8(0, d0.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(0, d0.secondsOnes);
  Digits dMax = secondsToDigits(MAX_SECONDS);
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
  TEST_ASSERT_EQUAL_UINT8(9, d.minutesTens);
  TEST_ASSERT_EQUAL_UINT8(9, d.secondsOnes);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_digits_use_standard_segment_bytes);
  RUN_TEST(test_digit_out_of_range_is_blank);
  RUN_TEST(test_known_letters);
  RUN_TEST(test_case_sensitive_where_shapes_differ);
  RUN_TEST(test_space_is_blank);
  RUN_TEST(test_illegible_or_unknown_is_dash);
  RUN_TEST(test_text_right_aligned_end);
  RUN_TEST(test_text_shorter_than_count_pads_blank);
  RUN_TEST(test_text_longer_than_count_truncates);
  RUN_TEST(test_blink_on_splits_period_in_half);
  RUN_TEST(test_blink_on_zero_period_is_always_on);
  RUN_TEST(test_seconds_to_digits_endpoints);
  RUN_TEST(test_seconds_to_digits_mid_value);
  RUN_TEST(test_seconds_to_digits_clamps_above_max);
  return UNITY_END();
}
