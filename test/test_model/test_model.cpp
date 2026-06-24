#include <unity.h>

#include "HobModel.h"

void setUp() {}
void tearDown() {}

void test_default_levels_are_zero() {
  HobModel m;
  for (uint8_t i = 0; i < m.hobCount(); ++i) TEST_ASSERT_EQUAL_UINT8(0, m.level(i));
}

void test_set_and_get_level() {
  HobModel m;
  m.setLevel(0, 200);
  m.setLevel(3, 50);
  TEST_ASSERT_EQUAL_UINT8(200, m.level(0));
  TEST_ASSERT_EQUAL_UINT8(50, m.level(3));
}

void test_out_of_range_hob_is_ignored() {
  HobModel m;
  m.setLevel(99, 123);                      // ignored, must not corrupt state or crash
  TEST_ASSERT_EQUAL_UINT8(0, m.level(99));  // out-of-range read returns 0
  TEST_ASSERT_EQUAL_UINT8(0, m.level(0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_levels_are_zero);
  RUN_TEST(test_set_and_get_level);
  RUN_TEST(test_out_of_range_hob_is_ignored);
  return UNITY_END();
}
