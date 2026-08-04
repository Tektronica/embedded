#include <unity.h>

#include "KeyDebounce.h"

using namespace keydebounce;
using et6226m::KeyPosition;

void setUp() {}
void tearDown() {}

bool isNoKey(KeyPosition k) { return k.grid == 0 && k.segment == 0; }

void test_no_key_reports_nothing() {
  Debouncer d;
  for (int i = 0; i < 5; ++i) TEST_ASSERT_TRUE(isNoKey(d.scan(KeyPosition{0, 0})));
}

void test_fresh_press_reports_key_after_debounce() {
  Debouncer d;
  KeyPosition last{0, 0};
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) last = d.scan(KeyPosition{2, 3});
  TEST_ASSERT_EQUAL_UINT8(2, last.grid);
  TEST_ASSERT_EQUAL_UINT8(3, last.segment);
}

void test_press_reports_only_once() {
  Debouncer d;
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) d.scan(KeyPosition{1, 1});
  TEST_ASSERT_TRUE(isNoKey(d.scan(KeyPosition{1, 1})));  // still held, no repeat
}

void test_bounce_before_debounce_threshold_reports_nothing() {
  Debouncer d;
  KeyPosition result{0, 0};
  bool bounced[] = {true, false, true, false};  // never DEBOUNCE_SCANS-consistent
  for (bool b : bounced) result = d.scan(b ? KeyPosition{1, 1} : KeyPosition{0, 0});
  TEST_ASSERT_TRUE(isNoKey(result));
}

void test_release_then_repress_reports_again() {
  Debouncer d;
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) d.scan(KeyPosition{4, 7});
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) d.scan(KeyPosition{0, 0});
  KeyPosition last{0, 0};
  for (int i = 0; i < DEBOUNCE_SCANS; ++i) last = d.scan(KeyPosition{4, 7});
  TEST_ASSERT_EQUAL_UINT8(4, last.grid);
  TEST_ASSERT_EQUAL_UINT8(7, last.segment);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_no_key_reports_nothing);
  RUN_TEST(test_fresh_press_reports_key_after_debounce);
  RUN_TEST(test_press_reports_only_once);
  RUN_TEST(test_bounce_before_debounce_threshold_reports_nothing);
  RUN_TEST(test_release_then_repress_reports_again);
  return UNITY_END();
}
