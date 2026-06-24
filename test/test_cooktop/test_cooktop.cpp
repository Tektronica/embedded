#include <unity.h>

#include "Cooktop.h"

void setUp() {}
void tearDown() {}

// --- HobModel ---

void test_default_levels_are_zero() {
  cooktop::HobModel m;
  for (uint8_t i = 0; i < m.hobCount(); ++i) TEST_ASSERT_EQUAL_UINT8(0, m.level(i));
}

void test_set_and_get_level() {
  cooktop::HobModel m;
  m.setLevel(0, 200);
  m.setLevel(3, 50);
  TEST_ASSERT_EQUAL_UINT8(200, m.level(0));
  TEST_ASSERT_EQUAL_UINT8(50, m.level(3));
}

void test_out_of_range_hob_is_ignored() {
  cooktop::HobModel m;
  m.setLevel(99, 123);                      // ignored, must not corrupt state or crash
  TEST_ASSERT_EQUAL_UINT8(0, m.level(99));  // out-of-range read returns 0
  TEST_ASSERT_EQUAL_UINT8(0, m.level(0));
}

// --- Input logic ---

void test_adc_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, cooktop::adcToLevel(0));
  TEST_ASSERT_EQUAL_UINT8(255, cooktop::adcToLevel(1023));
}

void test_adc_clamps_above_max() {
  TEST_ASSERT_EQUAL_UINT8(255, cooktop::adcToLevel(5000));
}

void test_ema_converges_toward_target() {
  uint16_t s = 0, prev = 0;
  for (int i = 0; i < 60; ++i) {
    s = cooktop::emaStep(s, 800, 3);
    TEST_ASSERT_TRUE(s >= prev);  // monotonic toward the target
    prev = s;
  }
  TEST_ASSERT_TRUE(s > 700);  // settles near 800
}

// --- Heat color ramp ---

void test_zero_level_is_off() { TEST_ASSERT_EQUAL_UINT8(0, cooktop::heatColor(0).v); }

void test_brightness_increases_with_level() {
  TEST_ASSERT_TRUE(cooktop::heatColor(255).v > cooktop::heatColor(20).v);
}

void test_hue_stays_in_red_orange_band() {
  TEST_ASSERT_EQUAL_UINT8(0, cooktop::heatColor(1).h);  // coolest = pure red (hue 0)
  TEST_ASSERT_TRUE(cooktop::heatColor(255).h <= 32);    // hottest = orange at most, never yellow
}

void test_color_is_always_fully_saturated() {           // red/orange, never washes out to white
  TEST_ASSERT_EQUAL_UINT8(255, cooktop::heatColor(40).s);
  TEST_ASSERT_EQUAL_UINT8(255, cooktop::heatColor(255).s);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_levels_are_zero);
  RUN_TEST(test_set_and_get_level);
  RUN_TEST(test_out_of_range_hob_is_ignored);
  RUN_TEST(test_adc_endpoints);
  RUN_TEST(test_adc_clamps_above_max);
  RUN_TEST(test_ema_converges_toward_target);
  RUN_TEST(test_zero_level_is_off);
  RUN_TEST(test_brightness_increases_with_level);
  RUN_TEST(test_hue_stays_in_red_orange_band);
  RUN_TEST(test_color_is_always_fully_saturated);
  return UNITY_END();
}
