#include <unity.h>

#include "Stepper.h"

using namespace stepper;

void setUp() {}
void tearDown() {}

void test_pot_to_speed_endpoints() {
  TEST_ASSERT_EQUAL_UINT16(0, potToSpeed(0));
  TEST_ASSERT_EQUAL_UINT16(MAX_SPEED_STEPS_PER_SEC, potToSpeed(1023));
}

void test_pot_to_speed_clamps_above_max() {
  TEST_ASSERT_EQUAL_UINT16(MAX_SPEED_STEPS_PER_SEC, potToSpeed(5000));
}

void test_ema_reaches_target_exactly() {
  uint16_t s = 0;
  for (int i = 0; i < 200; ++i) s = emaStep(s, 500, 3);
  TEST_ASSERT_EQUAL_UINT16(500, s);
}

void test_step_interval_matches_expected_frequency() {
  TEST_ASSERT_EQUAL_UINT32(1000, stepIntervalMicros(1000));  // 1000 steps/sec -> 1000 us apart
}

void test_step_interval_at_low_speed() {
  TEST_ASSERT_EQUAL_UINT32(1000000, stepIntervalMicros(1));  // 1 step/sec -> 1,000,000 us apart
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pot_to_speed_endpoints);
  RUN_TEST(test_pot_to_speed_clamps_above_max);
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_step_interval_matches_expected_frequency);
  RUN_TEST(test_step_interval_at_low_speed);
  return UNITY_END();
}
