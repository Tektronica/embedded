#include <unity.h>

#include "PotInput.h"

using namespace potinput;

void setUp() {}
void tearDown() {}

// --- EMA smoothing ---

void test_ema_reaches_target_exactly() {
  uint16_t s = 0;
  for (int i = 0; i < 200; ++i) s = emaStep(s, 800, 3);
  TEST_ASSERT_EQUAL_UINT16(800, s);
  for (int i = 0; i < 200; ++i) s = emaStep(s, ADC_MAX, 3);
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, s);
  for (int i = 0; i < 200; ++i) s = emaStep(s, 0, 3);
  TEST_ASSERT_EQUAL_UINT16(0, s);
}

// --- Percent scaling ---

void test_percent_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, toPercent(0));
  TEST_ASSERT_EQUAL_UINT8(PERCENT_MAX, toPercent(ADC_MAX));
}

void test_percent_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(PERCENT_MAX, toPercent(5000)); }

// --- Deadzone (defaults) ---

void test_deadzone_passes_through_midrange() {
  Deadzone dz;
  TEST_ASSERT_EQUAL_UINT16(500, dz.apply(500));
}

void test_deadzone_clamps_at_thresholds() {
  Deadzone dz;
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(DEADZONE_DEFAULT_LOW));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(DEADZONE_DEFAULT_HIGH));
}

void test_deadzone_low_holds_until_past_hysteresis() {
  Deadzone dz;
  dz.apply(DEADZONE_DEFAULT_LOW);
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(DEADZONE_DEFAULT_LOW + 1));
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(DEADZONE_DEFAULT_LOW + DEADZONE_DEFAULT_HYSTERESIS - 1));
  uint16_t released = DEADZONE_DEFAULT_LOW + DEADZONE_DEFAULT_HYSTERESIS;
  TEST_ASSERT_EQUAL_UINT16(released, dz.apply(released));
}

void test_deadzone_low_does_not_toggle_on_threshold_noise() {
  Deadzone dz;
  dz.apply(DEADZONE_DEFAULT_LOW);
  for (int i = 0; i < 10; ++i) {
    TEST_ASSERT_EQUAL_UINT16(0, dz.apply(DEADZONE_DEFAULT_LOW + 1));
    TEST_ASSERT_EQUAL_UINT16(0, dz.apply(DEADZONE_DEFAULT_LOW));
  }
}

void test_deadzone_high_holds_until_past_hysteresis() {
  Deadzone dz;
  dz.apply(DEADZONE_DEFAULT_HIGH);
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(DEADZONE_DEFAULT_HIGH - 1));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(DEADZONE_DEFAULT_HIGH - DEADZONE_DEFAULT_HYSTERESIS + 1));
  uint16_t released = DEADZONE_DEFAULT_HIGH - DEADZONE_DEFAULT_HYSTERESIS;
  TEST_ASSERT_EQUAL_UINT16(released, dz.apply(released));
}

// --- Deadzone (app-supplied thresholds) ---

void test_deadzone_uses_supplied_thresholds() {
  Deadzone dz(50, 900, 25);
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(50));
  TEST_ASSERT_EQUAL_UINT16(0, dz.apply(74));
  TEST_ASSERT_EQUAL_UINT16(75, dz.apply(75));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(900));
  TEST_ASSERT_EQUAL_UINT16(ADC_MAX, dz.apply(876));
  TEST_ASSERT_EQUAL_UINT16(875, dz.apply(875));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_percent_endpoints);
  RUN_TEST(test_percent_clamps_above_max);
  RUN_TEST(test_deadzone_passes_through_midrange);
  RUN_TEST(test_deadzone_clamps_at_thresholds);
  RUN_TEST(test_deadzone_low_holds_until_past_hysteresis);
  RUN_TEST(test_deadzone_low_does_not_toggle_on_threshold_noise);
  RUN_TEST(test_deadzone_high_holds_until_past_hysteresis);
  RUN_TEST(test_deadzone_uses_supplied_thresholds);
  return UNITY_END();
}
