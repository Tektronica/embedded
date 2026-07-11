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

// --- Perceived-brightness correction ---

void test_gamma_correct_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, gammaCorrect(0));
  TEST_ASSERT_EQUAL_UINT8(100, gammaCorrect(100));
}

void test_gamma_correct_curves_below_linear_midway() {
  TEST_ASSERT_EQUAL_UINT8(25, gammaCorrect(50));  // 50^2/100 = 25: dimmer than a linear 50%
}

void test_gamma_correct_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(100, gammaCorrect(150)); }

// --- Duty -> analogWrite() value (main_wokwi.cpp) ---

void test_duty_to_pwm8_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, dutyToPwm8(0));
  TEST_ASSERT_EQUAL_UINT8(255, dutyToPwm8(100));
}

void test_duty_to_pwm8_midpoint() { TEST_ASSERT_EQUAL_UINT8(127, dutyToPwm8(50)); }

void test_duty_to_pwm8_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(255, dutyToPwm8(150)); }

// --- Frequency -> Timer1 register math (main.cpp) ---

void test_default_frequency_uses_smallest_fitting_prescaler() {
  // 16 MHz / (1 * 1000 Hz) - 1 = 15999, already within the 16-bit TOP range -> Div1 wins (finest
  // duty resolution) over Div8/Div64/etc, since the search picks the smallest prescaler that fits.
  TimerConfig cfg = computeTimerConfig(16000000UL, 1000);
  TEST_ASSERT_TRUE(cfg.prescaler == Prescaler::Div1);
  TEST_ASSERT_EQUAL_UINT16(15999, cfg.top);
}

void test_low_frequency_uses_larger_prescaler() {
  // 16 MHz / (64 * 10 Hz) - 1 = 24999 is the smallest prescaler that keeps TOP <= 65536
  // (Div1 and Div8 would overflow it).
  TimerConfig cfg = computeTimerConfig(16000000UL, 10);
  TEST_ASSERT_TRUE(cfg.prescaler == Prescaler::Div64);
  TEST_ASSERT_EQUAL_UINT16(24999, cfg.top);
}

void test_high_frequency_uses_div1_prescaler() {
  // 16 MHz / (1 * 20000 Hz) - 1 = 799.
  TimerConfig cfg = computeTimerConfig(16000000UL, 20000);
  TEST_ASSERT_TRUE(cfg.prescaler == Prescaler::Div1);
  TEST_ASSERT_EQUAL_UINT16(799, cfg.top);
}

// --- Duty -> OCR1A (main.cpp) ---

void test_duty_to_ocr_endpoints() {
  TEST_ASSERT_EQUAL_UINT16(0, dutyToOcr(0, 1999));
  TEST_ASSERT_EQUAL_UINT16(2000, dutyToOcr(100, 1999));  // top+1 = full period = 100% on
}

void test_duty_to_ocr_midpoint() { TEST_ASSERT_EQUAL_UINT16(1000, dutyToOcr(50, 1999)); }

void test_duty_to_ocr_clamps_above_max() {
  TEST_ASSERT_EQUAL_UINT16(2000, dutyToOcr(150, 1999));
}

void test_duty_to_ocr_full_scale_top_does_not_wrap_to_zero() {
  // top == 0xFFFF: top+1 can't fit a 16-bit register, so 100% must settle for top (65535),
  // not wrap around to 0 (which would invert min and max duty).
  TEST_ASSERT_EQUAL_UINT16(65535, dutyToOcr(100, 65535));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_duty_endpoints);
  RUN_TEST(test_duty_clamps_above_max);
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_gamma_correct_endpoints);
  RUN_TEST(test_gamma_correct_curves_below_linear_midway);
  RUN_TEST(test_gamma_correct_clamps_above_max);
  RUN_TEST(test_duty_to_pwm8_endpoints);
  RUN_TEST(test_duty_to_pwm8_midpoint);
  RUN_TEST(test_duty_to_pwm8_clamps_above_max);
  RUN_TEST(test_default_frequency_uses_smallest_fitting_prescaler);
  RUN_TEST(test_low_frequency_uses_larger_prescaler);
  RUN_TEST(test_high_frequency_uses_div1_prescaler);
  RUN_TEST(test_duty_to_ocr_endpoints);
  RUN_TEST(test_duty_to_ocr_midpoint);
  RUN_TEST(test_duty_to_ocr_clamps_above_max);
  RUN_TEST(test_duty_to_ocr_full_scale_top_does_not_wrap_to_zero);
  return UNITY_END();
}
