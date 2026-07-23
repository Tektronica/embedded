#include <unity.h>

#include "Pwm.h"

using namespace pwm;

void setUp() {}
void tearDown() {}

// --- Potentiometer input ---

void test_duty_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, dutyFromAdc(0));
  TEST_ASSERT_EQUAL_UINT8(100, dutyFromAdc(1023));
}

void test_duty_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(100, dutyFromAdc(5000)); }

void test_ema_reaches_target_exactly() {
  uint16_t s = 0;
  for (int i = 0; i < 200; ++i) s = emaStep(s, 800, 3);
  TEST_ASSERT_EQUAL_UINT16(800, s);
  for (int i = 0; i < 200; ++i) s = emaStep(s, 1023, 3);
  TEST_ASSERT_EQUAL_UINT16(1023, s);  // pot max reaches the rail -> 100% duty
}

// --- Duty -> analogWrite() value ---

void test_duty_to_pwm8_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, dutyToPwm8(0));
  TEST_ASSERT_EQUAL_UINT8(255, dutyToPwm8(100));
}

void test_duty_to_pwm8_midpoint() { TEST_ASSERT_EQUAL_UINT8(127, dutyToPwm8(50)); }

void test_duty_to_pwm8_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(255, dutyToPwm8(150)); }

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_duty_endpoints);
  RUN_TEST(test_duty_clamps_above_max);
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_duty_to_pwm8_endpoints);
  RUN_TEST(test_duty_to_pwm8_midpoint);
  RUN_TEST(test_duty_to_pwm8_clamps_above_max);
  return UNITY_END();
}
