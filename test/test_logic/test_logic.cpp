#include <unity.h>

#include "HeatRamp.h"
#include "InputMap.h"

void setUp() {}
void tearDown() {}

// --- InputMap ---

void test_adc_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, inputmap::adcToLevel(0));
  TEST_ASSERT_EQUAL_UINT8(255, inputmap::adcToLevel(1023));
}

void test_adc_clamps_above_max() {
  TEST_ASSERT_EQUAL_UINT8(255, inputmap::adcToLevel(5000));
}

void test_ema_converges_toward_target() {
  uint16_t s = 0;
  uint16_t prev = 0;
  for (int i = 0; i < 60; ++i) {
    s = inputmap::emaStep(s, 800, 3);
    TEST_ASSERT_TRUE(s >= prev);  // monotonic toward the target
    prev = s;
  }
  TEST_ASSERT_TRUE(s > 700);  // settles near 800
}

// --- HeatRamp ---

void test_zero_level_is_off() {
  heatramp::Hsv c = heatramp::colorFor(0);
  TEST_ASSERT_EQUAL_UINT8(0, c.v);
}

void test_brightness_increases_with_level() {
  TEST_ASSERT_TRUE(heatramp::colorFor(255).v > heatramp::colorFor(20).v);
}

void test_hue_stays_in_red_to_yellow_band() {
  TEST_ASSERT_TRUE(heatramp::colorFor(1).h <= 64);
  TEST_ASSERT_TRUE(heatramp::colorFor(128).h <= 64);
  TEST_ASSERT_TRUE(heatramp::colorFor(255).h <= 64);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_adc_endpoints);
  RUN_TEST(test_adc_clamps_above_max);
  RUN_TEST(test_ema_converges_toward_target);
  RUN_TEST(test_zero_level_is_off);
  RUN_TEST(test_brightness_increases_with_level);
  RUN_TEST(test_hue_stays_in_red_to_yellow_band);
  return UNITY_END();
}
