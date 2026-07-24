#include <unity.h>

#include "Score.h"

using namespace score;

void setUp() {}
void tearDown() {}

void test_compute_scales_with_elapsed_time_and_speed() {
  TEST_ASSERT_EQUAL_UINT16(0, compute(0, 8));
  TEST_ASSERT_EQUAL_UINT16(8, compute(1000, 8));
  TEST_ASSERT_EQUAL_UINT16(100, compute(12500, 8));
}

void test_milestone_unchanged_below_the_next_interval() {
  TEST_ASSERT_EQUAL_UINT16(0, milestoneFor(99, 0));
}

void test_milestone_advances_once_score_crosses_interval() {
  TEST_ASSERT_EQUAL_UINT16(1, milestoneFor(100, 0));
}

void test_milestone_does_not_re_fire_within_the_same_interval() {
  TEST_ASSERT_EQUAL_UINT16(1, milestoneFor(150, 1));
}

void test_milestone_advances_again_at_the_next_interval() {
  TEST_ASSERT_EQUAL_UINT16(2, milestoneFor(200, 1));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_compute_scales_with_elapsed_time_and_speed);
  RUN_TEST(test_milestone_unchanged_below_the_next_interval);
  RUN_TEST(test_milestone_advances_once_score_crosses_interval);
  RUN_TEST(test_milestone_does_not_re_fire_within_the_same_interval);
  RUN_TEST(test_milestone_advances_again_at_the_next_interval);
  return UNITY_END();
}
