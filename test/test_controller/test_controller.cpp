#include <unity.h>

#include "LEDStripDimmer.h"

void setUp() {}
void tearDown() {}

// --- Levels ---

void test_default_levels_are_zero() {
  controller::Levels m;
  for (uint8_t ch = 0; ch < m.count(); ++ch) TEST_ASSERT_EQUAL_UINT8(0, m.level(ch));
}

void test_set_and_get_level() {
  controller::Levels m;
  m.setLevel(0, 200);
  m.setLevel(3, 50);
  TEST_ASSERT_EQUAL_UINT8(200, m.level(0));
  TEST_ASSERT_EQUAL_UINT8(50, m.level(3));
}

void test_out_of_range_channel_is_ignored() {
  controller::Levels m;
  m.setLevel(99, 123);                      // ignored, must not corrupt state or crash
  TEST_ASSERT_EQUAL_UINT8(0, m.level(99));  // out-of-range read returns 0
  TEST_ASSERT_EQUAL_UINT8(0, m.level(0));
}

// --- Dimmer input logic ---

void test_adc_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, controller::adcToLevel(0));
  TEST_ASSERT_EQUAL_UINT8(255, controller::adcToLevel(1023));
}

void test_adc_clamps_above_max() {
  TEST_ASSERT_EQUAL_UINT8(255, controller::adcToLevel(5000));
}

void test_ema_reaches_target_exactly() {
  uint16_t s = 0;
  for (int i = 0; i < 200; ++i) s = controller::emaStep(s, 800, 3);
  TEST_ASSERT_EQUAL_UINT16(800, s);   // converges exactly — no dead-band
  for (int i = 0; i < 200; ++i) s = controller::emaStep(s, 1023, 3);
  TEST_ASSERT_EQUAL_UINT16(1023, s);  // reaches the max rail -> dimmer max = full brightness
}

// --- Level → color curve ---

void test_zero_level_is_off() { TEST_ASSERT_EQUAL_UINT8(0, controller::levelColor(0).v); }

void test_brightness_increases_with_level() {
  TEST_ASSERT_TRUE(controller::levelColor(255).v > controller::levelColor(20).v);
}

void test_hue_stays_in_red_orange_band() {
  TEST_ASSERT_EQUAL_UINT8(0, controller::levelColor(1).h);  // coolest = pure red (hue 0)
  TEST_ASSERT_TRUE(controller::levelColor(255).h <= 16);    // hottest = orange-red, never plain orange/yellow
}

void test_color_is_always_fully_saturated() {               // never washes out to white
  TEST_ASSERT_EQUAL_UINT8(255, controller::levelColor(40).s);
  TEST_ASSERT_EQUAL_UINT8(255, controller::levelColor(255).s);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_levels_are_zero);
  RUN_TEST(test_set_and_get_level);
  RUN_TEST(test_out_of_range_channel_is_ignored);
  RUN_TEST(test_adc_endpoints);
  RUN_TEST(test_adc_clamps_above_max);
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_zero_level_is_off);
  RUN_TEST(test_brightness_increases_with_level);
  RUN_TEST(test_hue_stays_in_red_orange_band);
  RUN_TEST(test_color_is_always_fully_saturated);
  return UNITY_END();
}
