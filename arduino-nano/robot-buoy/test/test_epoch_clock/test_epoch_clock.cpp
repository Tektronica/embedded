#include <unity.h>

#include "EpochClock.h"

using namespace epoch;

void setUp() {}
void tearDown() {}

void test_starts_unsynced() {
  Clock c;
  TEST_ASSERT_FALSE(c.isSynced());
  TEST_ASSERT_EQUAL_UINT32(0, c.current(12345));
}

void test_sync_sets_current_value() {
  Clock c;
  c.sync(1000, 500);
  TEST_ASSERT_TRUE(c.isSynced());
  TEST_ASSERT_EQUAL_UINT32(1000, c.current(500));
}

void test_current_extrapolates_elapsed_millis() {
  Clock c;
  c.sync(1000, 500);
  TEST_ASSERT_EQUAL_UINT32(1005, c.current(500 + 5000));  // 5000 ms later = 5 s later
}

void test_resync_replaces_previous_estimate() {
  Clock c;
  c.sync(1000, 0);
  c.sync(5000, 100);
  TEST_ASSERT_EQUAL_UINT32(5000, c.current(100));
}

void test_unsynced_clock_accepts_anything_as_plausible() {
  Clock c;
  TEST_ASSERT_TRUE(isPlausible(c, 999999, 0, 60));
}

void test_synced_clock_rejects_out_of_tolerance_value() {
  Clock c;
  c.sync(1000, 0);
  TEST_ASSERT_FALSE(isPlausible(c, 1000 + 61, 0, 60));
}

void test_synced_clock_accepts_within_tolerance_value() {
  Clock c;
  c.sync(1000, 0);
  TEST_ASSERT_TRUE(isPlausible(c, 1000 + 60, 0, 60));
}

void test_resync_below_min_interval_does_not_learn_drift() {
  Clock c;
  c.sync(1000, 0);
  // 40s of local millis elapsed but the reference says 41s passed (a plausible 1.025x ratio if
  // it were measured) -- below the 60s minimum interval, so the correction should stay at 1.0.
  c.sync(1041, 40000);
  TEST_ASSERT_EQUAL_UINT32(1101, c.current(100000));  // 1041 + 60s uncorrected, not 1102
}

void test_resync_above_min_interval_learns_slow_clock_drift() {
  Clock c;
  c.sync(1000, 0);
  // 100s of local millis elapsed but the reference says 102s passed -- local clock is slow,
  // learn a 1.02x correction.
  c.sync(1102, 100000);
  TEST_ASSERT_EQUAL_UINT32(1153, c.current(150000));  // 1102 + 51s corrected, not 1102 + 50s
}

void test_resync_rejects_implausible_drift_ratio() {
  Clock c;
  c.sync(1000, 0);
  // 100s of local millis elapsed but the reference says 200s passed -- a 2.0x ratio is way
  // outside real crystal drift, so it's rejected and the correction stays at 1.0.
  c.sync(1200, 100000);
  TEST_ASSERT_EQUAL_UINT32(1250, c.current(150000));  // 1200 + 50s uncorrected, not 1200 + 100s
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_unsynced);
  RUN_TEST(test_sync_sets_current_value);
  RUN_TEST(test_current_extrapolates_elapsed_millis);
  RUN_TEST(test_resync_replaces_previous_estimate);
  RUN_TEST(test_unsynced_clock_accepts_anything_as_plausible);
  RUN_TEST(test_synced_clock_rejects_out_of_tolerance_value);
  RUN_TEST(test_synced_clock_accepts_within_tolerance_value);
  RUN_TEST(test_resync_below_min_interval_does_not_learn_drift);
  RUN_TEST(test_resync_above_min_interval_learns_slow_clock_drift);
  RUN_TEST(test_resync_rejects_implausible_drift_ratio);
  return UNITY_END();
}
