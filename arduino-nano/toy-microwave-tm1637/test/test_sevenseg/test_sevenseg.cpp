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

void test_encode_char_letters_used_in_end() {
  // Regression check: these are the exact values main.cpp hardcoded for "End" before this table
  // existed.
  TEST_ASSERT_EQUAL_HEX8(0x79, encodeChar('E'));
  TEST_ASSERT_EQUAL_HEX8(0x54, encodeChar('n'));
  TEST_ASSERT_EQUAL_HEX8(0x5E, encodeChar('d'));
}

void test_encode_char_space_and_unsupported_are_blank() {
  TEST_ASSERT_EQUAL_HEX8(0x00, encodeChar(' '));
  TEST_ASSERT_EQUAL_HEX8(0x00, encodeChar('!'));
  TEST_ASSERT_EQUAL_HEX8(0x00, encodeChar('~'));
}

void test_encode_char_is_case_invariant() {
  // A real seven-segment display has exactly one shape per letter -- 'D' and 'd' must return the
  // same value, since there's no separate uppercase/lowercase font on the hardware itself.
  const char* pairs = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
  for (uint8_t i = 0; pairs[i] != '\0'; i += 2) {
    TEST_ASSERT_EQUAL_HEX8(encodeChar(pairs[i]), encodeChar(pairs[i + 1]));
  }
}

void test_encode_char_d_does_not_collide_with_zero() {
  // D's canonical rendering is the lowercase-style glyph, not the closed loop that collides with
  // digit 0 -- unlike O, which has no distinct alternative and does collide.
  TEST_ASSERT_NOT_EQUAL(encodeChar('0'), encodeChar('D'));
  TEST_ASSERT_EQUAL_HEX8(encodeChar('0'), encodeChar('O'));
}

void test_encode_text_any_casing_of_done_matches() {
  // The exact regression this table was built to satisfy: DONE, done, DoNe, and dOnE must all
  // render identically.
  uint8_t a[4], b[4], c[4], d[4];
  encodeText("DONE", a, 4);
  encodeText("done", b, 4);
  encodeText("DoNe", c, 4);
  encodeText("dOnE", d, 4);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(a, b, 4);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(a, c, 4);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(a, d, 4);
}

void test_encode_text_right_aligned_end() {
  uint8_t segments[4];
  encodeText(" End", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(0x00, segments[0]);
  TEST_ASSERT_EQUAL_HEX8(0x79, segments[1]);
  TEST_ASSERT_EQUAL_HEX8(0x54, segments[2]);
  TEST_ASSERT_EQUAL_HEX8(0x5E, segments[3]);
}

void test_encode_text_exact_length_done() {
  uint8_t segments[4];
  encodeText("Done", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('D'), segments[0]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('o'), segments[1]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('n'), segments[2]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('e'), segments[3]);
}

void test_encode_text_shorter_than_count_blanks_trailing() {
  uint8_t segments[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  encodeText("Hi", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('H'), segments[0]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('i'), segments[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, segments[2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, segments[3]);
}

void test_encode_text_longer_than_count_truncates() {
  uint8_t segments[4];
  encodeText("Escape", segments, 4);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('E'), segments[0]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('s'), segments[1]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('c'), segments[2]);
  TEST_ASSERT_EQUAL_HEX8(encodeChar('a'), segments[3]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_seconds_to_digits_endpoints);
  RUN_TEST(test_seconds_to_digits_mid_value);
  RUN_TEST(test_seconds_to_digits_clamps_above_max);
  RUN_TEST(test_blink_on_splits_period_in_half);
  RUN_TEST(test_blink_on_zero_period_is_always_on);
  RUN_TEST(test_encode_char_letters_used_in_end);
  RUN_TEST(test_encode_char_space_and_unsupported_are_blank);
  RUN_TEST(test_encode_char_is_case_invariant);
  RUN_TEST(test_encode_char_d_does_not_collide_with_zero);
  RUN_TEST(test_encode_text_any_casing_of_done_matches);
  RUN_TEST(test_encode_text_right_aligned_end);
  RUN_TEST(test_encode_text_exact_length_done);
  RUN_TEST(test_encode_text_shorter_than_count_blanks_trailing);
  RUN_TEST(test_encode_text_longer_than_count_truncates);
  return UNITY_END();
}
