#include <unity.h>

#include "ET6226MCodec.h"

using namespace et6226m;

void setUp() {}
void tearDown() {}

void test_encode_digit_zero_through_nine() {
  TEST_ASSERT_EQUAL_HEX8(0x3F, encodeDigit(0));
  TEST_ASSERT_EQUAL_HEX8(0x06, encodeDigit(1));
  TEST_ASSERT_EQUAL_HEX8(0x5B, encodeDigit(2));
  TEST_ASSERT_EQUAL_HEX8(0x4F, encodeDigit(3));
  TEST_ASSERT_EQUAL_HEX8(0x66, encodeDigit(4));
  TEST_ASSERT_EQUAL_HEX8(0x6D, encodeDigit(5));
  TEST_ASSERT_EQUAL_HEX8(0x7D, encodeDigit(6));
  TEST_ASSERT_EQUAL_HEX8(0x07, encodeDigit(7));
  TEST_ASSERT_EQUAL_HEX8(0x7F, encodeDigit(8));
  TEST_ASSERT_EQUAL_HEX8(0x6F, encodeDigit(9));
}

void test_encode_digit_out_of_range_is_blank() {
  TEST_ASSERT_EQUAL_HEX8(0x00, encodeDigit(10));
  TEST_ASSERT_EQUAL_HEX8(0x00, encodeDigit(255));
}

void test_decode_key_code_zero_is_no_key() {
  KeyPosition key = decodeKeyCode(0x00);
  TEST_ASSERT_EQUAL_UINT8(0, key.grid);
  TEST_ASSERT_EQUAL_UINT8(0, key.segment);
}

void test_decode_key_code_first_and_last_entries() {
  // SG1/GR1 = 0x44, the datasheet's first table entry.
  KeyPosition first = decodeKeyCode(0x44);
  TEST_ASSERT_EQUAL_UINT8(1, first.grid);
  TEST_ASSERT_EQUAL_UINT8(1, first.segment);

  // SG7/GR4 = 0x77, the datasheet's last table entry.
  KeyPosition last = decodeKeyCode(0x77);
  TEST_ASSERT_EQUAL_UINT8(4, last.grid);
  TEST_ASSERT_EQUAL_UINT8(7, last.segment);
}

void test_decode_key_code_matches_table_middle_entries() {
  // SG2/GR4 = 0x4F -- same byte value as the Key Code Command address itself, a datasheet
  // coincidence worth a dedicated test since it's easy to mix up the two meanings.
  KeyPosition sg2gr4 = decodeKeyCode(0x4F);
  TEST_ASSERT_EQUAL_UINT8(4, sg2gr4.grid);
  TEST_ASSERT_EQUAL_UINT8(2, sg2gr4.segment);

  // SG6/GR3 = 0x6E.
  KeyPosition sg6gr3 = decodeKeyCode(0x6E);
  TEST_ASSERT_EQUAL_UINT8(3, sg6gr3.grid);
  TEST_ASSERT_EQUAL_UINT8(6, sg6gr3.segment);
}

void test_decode_key_code_rejects_below_table_range() {
  KeyPosition key = decodeKeyCode(0x43);
  TEST_ASSERT_EQUAL_UINT8(0, key.grid);
  TEST_ASSERT_EQUAL_UINT8(0, key.segment);
}

void test_decode_key_code_rejects_gaps_between_grid_columns() {
  // 0x48-0x4B fall between GR4 (0x47) and the next segment's GR1 (0x4C) -- not a real code.
  // 0x48 is also the Display Control Command's address, another datasheet coincidence worth a
  // dedicated test for the same reason as the 0x4F one above.
  KeyPosition key = decodeKeyCode(0x48);
  TEST_ASSERT_EQUAL_UINT8(0, key.grid);
  TEST_ASSERT_EQUAL_UINT8(0, key.segment);
}

void test_encode_display_control_matches_datasheet_examples() {
  // The datasheet's own examples: "X9H" is 7-segment mode, "X1H" is 8-segment mode (brightness=0,
  // display on, in both cases).
  TEST_ASSERT_EQUAL_HEX8(0x09, encodeDisplayControl(0, true, SegmentMode::SevenSegment));
  TEST_ASSERT_EQUAL_HEX8(0x01, encodeDisplayControl(0, true, SegmentMode::EightSegment));
}

void test_encode_display_control_brightness_and_display_off() {
  TEST_ASSERT_EQUAL_HEX8(0x79, encodeDisplayControl(MAX_BRIGHTNESS, true, SegmentMode::SevenSegment));
  TEST_ASSERT_EQUAL_HEX8(0x08, encodeDisplayControl(0, false, SegmentMode::SevenSegment));
}

void test_encode_display_control_clamps_brightness() {
  TEST_ASSERT_EQUAL_HEX8(encodeDisplayControl(MAX_BRIGHTNESS, true, SegmentMode::SevenSegment),
                          encodeDisplayControl(255, true, SegmentMode::SevenSegment));
}

void test_decode_key_code_rejects_above_table_range() {
  KeyPosition key = decodeKeyCode(0x78);
  TEST_ASSERT_EQUAL_UINT8(0, key.grid);
  TEST_ASSERT_EQUAL_UINT8(0, key.segment);
}

void test_encode_char_digits_match_encode_digit() {
  for (uint8_t d = 0; d <= 9; ++d) {
    TEST_ASSERT_EQUAL_HEX8(encodeDigit(d), encodeChar(static_cast<char>('0' + d)));
  }
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
  TEST_ASSERT_NOT_EQUAL(encodeDigit(0), encodeChar('D'));
  TEST_ASSERT_EQUAL_HEX8(encodeDigit(0), encodeChar('O'));
}

void test_encode_text_any_casing_of_done_matches() {
  // The exact regression this project was built to satisfy: DONE, done, DoNe, and dOnE must all
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
  RUN_TEST(test_encode_digit_zero_through_nine);
  RUN_TEST(test_encode_digit_out_of_range_is_blank);
  RUN_TEST(test_decode_key_code_zero_is_no_key);
  RUN_TEST(test_decode_key_code_first_and_last_entries);
  RUN_TEST(test_decode_key_code_matches_table_middle_entries);
  RUN_TEST(test_decode_key_code_rejects_below_table_range);
  RUN_TEST(test_decode_key_code_rejects_gaps_between_grid_columns);
  RUN_TEST(test_decode_key_code_rejects_above_table_range);
  RUN_TEST(test_encode_char_digits_match_encode_digit);
  RUN_TEST(test_encode_char_letters_used_in_end);
  RUN_TEST(test_encode_char_space_and_unsupported_are_blank);
  RUN_TEST(test_encode_char_is_case_invariant);
  RUN_TEST(test_encode_char_d_does_not_collide_with_zero);
  RUN_TEST(test_encode_text_any_casing_of_done_matches);
  RUN_TEST(test_encode_text_right_aligned_end);
  RUN_TEST(test_encode_text_exact_length_done);
  RUN_TEST(test_encode_text_shorter_than_count_blanks_trailing);
  RUN_TEST(test_encode_text_longer_than_count_truncates);
  RUN_TEST(test_encode_display_control_matches_datasheet_examples);
  RUN_TEST(test_encode_display_control_brightness_and_display_off);
  RUN_TEST(test_encode_display_control_clamps_brightness);
  return UNITY_END();
}
