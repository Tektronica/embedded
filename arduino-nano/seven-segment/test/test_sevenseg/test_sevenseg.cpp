#include <unity.h>

#include "SevenSegment.h"

using namespace sevenseg;
using namespace segdisplay;

void setUp() {}
void tearDown() {}

void test_roll_offset_advances_one_step_per_period() {
  TEST_ASSERT_EQUAL_UINT16(0, rollOffset(5, 0, 3));
  TEST_ASSERT_EQUAL_UINT16(0, rollOffset(5, 2, 3));
  TEST_ASSERT_EQUAL_UINT16(1, rollOffset(5, 3, 3));
}

void test_roll_offset_wraps_after_label_plus_window() {
  // labelLength=5, WINDOW=4 -> cycle length 9
  TEST_ASSERT_EQUAL_UINT16(0, rollOffset(5, 9 * 3, 3));
}

void test_render_static_number_shows_all_four_digits() {
  Segments s = render(numberContent(1234), Mode::Static, 0, 0);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('1'), s.values[0]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('2'), s.values[1]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('3'), s.values[2]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('4'), s.values[3]);
}

void test_render_static_number_clamps_above_9999() {
  Segments s = render(numberContent(60000), Mode::Static, 0, 0);
  Segments max = render(numberContent(9999), Mode::Static, 0, 0);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(max.values, s.values, 4);
}

void test_render_static_label_pads_with_blanks() {
  Segments s = render(labelContent("OK", 2), Mode::Static, 0, 0);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('O'), s.values[0]) ;
  TEST_ASSERT_EQUAL_UINT8(encodeChar('K'), s.values[1]);  // 'K' isn't renderable -> DASH
  TEST_ASSERT_EQUAL_UINT8(BLANK, s.values[2]);
  TEST_ASSERT_EQUAL_UINT8(BLANK, s.values[3]);
}

void test_render_flashing_blanks_during_off_phase() {
  Segments on  = render(numberContent(8), Mode::Flashing, 0, 10);
  Segments off = render(numberContent(8), Mode::Flashing, 5, 10);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('8'), on.values[3]);
  TEST_ASSERT_EQUAL_UINT8(BLANK, off.values[0]);
  TEST_ASSERT_EQUAL_UINT8(BLANK, off.values[3]);
}

void test_render_rolling_label_shows_leading_window_at_start() {
  Segments s = render(labelContent("HELLO", 5), Mode::Rolling, 0, 3);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('H'), s.values[0]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('E'), s.values[1]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('L'), s.values[2]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('L'), s.values[3]);
}

void test_render_rolling_label_advances_the_window() {
  Segments s = render(labelContent("HELLO", 5), Mode::Rolling, 3, 3);  // one step in
  TEST_ASSERT_EQUAL_UINT8(encodeChar('E'), s.values[0]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('L'), s.values[1]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('L'), s.values[2]);
  TEST_ASSERT_EQUAL_UINT8(encodeChar('O'), s.values[3]);
}

void test_render_rolling_number_falls_back_to_static() {
  Segments rolling = render(numberContent(42), Mode::Rolling, 7, 3);
  Segments stat     = render(numberContent(42), Mode::Static, 0, 0);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(stat.values, rolling.values, 4);
}

void test_with_colon_ors_the_dp_bit_into_the_given_digit() {
  Segments s = render(numberContent(1234), Mode::Static, 0, 0);
  Segments withColonOn = withColon(s, 1, true);
  TEST_ASSERT_EQUAL_UINT8(s.values[0], withColonOn.values[0]);
  TEST_ASSERT_EQUAL_UINT8(s.values[1] | SEGBIT_DP, withColonOn.values[1]);
  TEST_ASSERT_EQUAL_UINT8(s.values[2], withColonOn.values[2]);
}

void test_with_colon_off_leaves_segments_unchanged() {
  Segments s = render(numberContent(1234), Mode::Static, 0, 0);
  Segments withColonOff = withColon(s, 1, false);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(s.values, withColonOff.values, 4);
}

void test_with_colon_composes_with_flashing() {
  // The colon can blink on its own schedule independent of the digits' own Flashing state.
  Segments digitsOff = render(numberContent(1234), Mode::Flashing, 5, 10);  // digits blanked
  Segments s = withColon(digitsOff, 1, true);
  TEST_ASSERT_EQUAL_UINT8(BLANK | SEGBIT_DP, s.values[1]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_roll_offset_advances_one_step_per_period);
  RUN_TEST(test_roll_offset_wraps_after_label_plus_window);
  RUN_TEST(test_render_static_number_shows_all_four_digits);
  RUN_TEST(test_render_static_number_clamps_above_9999);
  RUN_TEST(test_render_static_label_pads_with_blanks);
  RUN_TEST(test_render_flashing_blanks_during_off_phase);
  RUN_TEST(test_render_rolling_label_shows_leading_window_at_start);
  RUN_TEST(test_render_rolling_label_advances_the_window);
  RUN_TEST(test_render_rolling_number_falls_back_to_static);
  RUN_TEST(test_with_colon_ors_the_dp_bit_into_the_given_digit);
  RUN_TEST(test_with_colon_off_leaves_segments_unchanged);
  RUN_TEST(test_with_colon_composes_with_flashing);
  return UNITY_END();
}
