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

// --- Deadzone ---

void test_deadzone_passes_through_midrange() {
  Deadzone dz;
  TEST_ASSERT_EQUAL_UINT16(500, dz.apply(500));
}

void test_deadzone_clamps_at_thresholds() {
  Deadzone dz;
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(ADC_DEADZONE_LOW));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(ADC_DEADZONE_HIGH));
}

void test_deadzone_low_holds_until_past_hysteresis() {
  Deadzone dz;
  dz.apply(ADC_DEADZONE_LOW);
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(ADC_DEADZONE_LOW + 1));
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(ADC_DEADZONE_LOW + DEADZONE_HYSTERESIS - 1));
  uint16_t released = ADC_DEADZONE_LOW + DEADZONE_HYSTERESIS;
  TEST_ASSERT_EQUAL_UINT16(released, dz.apply(released));
}

void test_deadzone_low_does_not_toggle_on_threshold_noise() {
  Deadzone dz;
  dz.apply(ADC_DEADZONE_LOW);
  for (int i = 0; i < 10; ++i) {
    TEST_ASSERT_EQUAL_UINT16(0, dz.apply(ADC_DEADZONE_LOW + 1));
    TEST_ASSERT_EQUAL_UINT16(0, dz.apply(ADC_DEADZONE_LOW));
  }
}

void test_deadzone_high_holds_until_past_hysteresis() {
  Deadzone dz;
  dz.apply(ADC_DEADZONE_HIGH);
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(ADC_DEADZONE_HIGH - 1));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(ADC_DEADZONE_HIGH - DEADZONE_HYSTERESIS + 1));
  uint16_t released = ADC_DEADZONE_HIGH - DEADZONE_HYSTERESIS;
  TEST_ASSERT_EQUAL_UINT16(released, dz.apply(released));
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
  RUN_TEST(test_deadzone_passes_through_midrange);
  RUN_TEST(test_deadzone_clamps_at_thresholds);
  RUN_TEST(test_deadzone_low_holds_until_past_hysteresis);
  RUN_TEST(test_deadzone_low_does_not_toggle_on_threshold_noise);
  RUN_TEST(test_deadzone_high_holds_until_past_hysteresis);
  RUN_TEST(test_duty_to_pwm8_endpoints);
  RUN_TEST(test_duty_to_pwm8_midpoint);
  RUN_TEST(test_duty_to_pwm8_clamps_above_max);
  return UNITY_END();
}
