#include <unity.h>

#include "ET6226MCodec.h"

using namespace et6226m;

void setUp() {}
void tearDown() {}

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
  // The datasheet's own examples: "X9H" is 7-segment mode, "X1H" is 8-segment mode (X=0 is the
  // brightest step, display on, in both cases).
  TEST_ASSERT_EQUAL_HEX8(0x09, encodeDisplayControl(MAX_BRIGHTNESS, true, SegmentMode::SevenSegment));
  TEST_ASSERT_EQUAL_HEX8(0x01, encodeDisplayControl(MAX_BRIGHTNESS, true, SegmentMode::EightSegment));
}

void test_encode_display_control_brightness_and_display_off() {
  TEST_ASSERT_EQUAL_HEX8(0x19, encodeDisplayControl(0, true, SegmentMode::SevenSegment));  // "1 step"
  TEST_ASSERT_EQUAL_HEX8(0x79, encodeDisplayControl(6, true, SegmentMode::SevenSegment));  // "7 step"
  TEST_ASSERT_EQUAL_HEX8(0x18, encodeDisplayControl(0, false, SegmentMode::SevenSegment));
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


void test_key_position_equality() {
  TEST_ASSERT_TRUE((KeyPosition{2, 3} == KeyPosition{2, 3}));
  TEST_ASSERT_FALSE((KeyPosition{2, 3} == KeyPosition{3, 2}));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_key_position_equality);
  RUN_TEST(test_decode_key_code_zero_is_no_key);
  RUN_TEST(test_decode_key_code_first_and_last_entries);
  RUN_TEST(test_decode_key_code_matches_table_middle_entries);
  RUN_TEST(test_decode_key_code_rejects_below_table_range);
  RUN_TEST(test_decode_key_code_rejects_gaps_between_grid_columns);
  RUN_TEST(test_decode_key_code_rejects_above_table_range);
  RUN_TEST(test_encode_display_control_matches_datasheet_examples);
  RUN_TEST(test_encode_display_control_brightness_and_display_off);
  RUN_TEST(test_encode_display_control_clamps_brightness);
  return UNITY_END();
}
