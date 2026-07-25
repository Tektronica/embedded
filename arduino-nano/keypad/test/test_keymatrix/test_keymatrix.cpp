#include <unity.h>

#include "KeyMatrix.h"

using namespace keymatrix;

void setUp() {}
void tearDown() {}

void test_no_key_reports_nothing() {
  Scanner s;
  for (int i = 0; i < 5; ++i) TEST_ASSERT_EQUAL_CHAR('\0', s.scan(NO_KEY));
}

void test_fresh_press_reports_key_after_debounce() {
  Scanner s;
  char last = '\0';
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) last = s.scan(0);  // row 0, col 0 -> '1'
  TEST_ASSERT_EQUAL_CHAR('1', last);
}

void test_press_reports_only_once() {
  Scanner s;
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) s.scan(5);  // row 1, col 1 -> '5'
  TEST_ASSERT_EQUAL_CHAR('\0', s.scan(5));             // still held, no repeat
}

void test_bounce_before_debounce_threshold_reports_nothing() {
  Scanner s;
  char result = '\0';
  bool bounced[] = {true, false, true, false};  // never DEBOUNCE_SCANS-consistent
  for (bool b : bounced) result = s.scan(b ? 0 : NO_KEY);
  TEST_ASSERT_EQUAL_CHAR('\0', result);
}

void test_release_then_repress_reports_again() {
  Scanner s;
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) s.scan(14);  // row 3, col 2 -> '#'
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) s.scan(NO_KEY);
  char last = '\0';
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) last = s.scan(14);
  TEST_ASSERT_EQUAL_CHAR('#', last);
}

void test_layout_matches_standard_membrane_keypad() {
  TEST_ASSERT_EQUAL_CHAR('1', LAYOUT[0][0]);
  TEST_ASSERT_EQUAL_CHAR('D', LAYOUT[3][3]);
  TEST_ASSERT_EQUAL_CHAR('0', LAYOUT[3][1]);
  TEST_ASSERT_EQUAL_CHAR('#', LAYOUT[3][2]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_no_key_reports_nothing);
  RUN_TEST(test_fresh_press_reports_key_after_debounce);
  RUN_TEST(test_press_reports_only_once);
  RUN_TEST(test_bounce_before_debounce_threshold_reports_nothing);
  RUN_TEST(test_release_then_repress_reports_again);
  RUN_TEST(test_layout_matches_standard_membrane_keypad);
  return UNITY_END();
}
