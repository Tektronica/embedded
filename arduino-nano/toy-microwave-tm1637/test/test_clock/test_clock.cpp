#include <unity.h>

#include "Clock.h"

using namespace wallclock;

void setUp() {}
void tearDown() {}

void test_starts_at_midnight() {
  Clock c;
  TEST_ASSERT_EQUAL_UINT16(0, c.minutesOfDay());
}

void test_tick_advances_one_minute_after_sixty_ticks() {
  Clock c;
  for (int i = 0; i < 60; ++i) c.tick();
  TEST_ASSERT_EQUAL_UINT16(1, c.minutesOfDay());
}

void test_tick_wraps_at_midnight() {
  Clock c;
  c.setMinutesOfDay(23 * 60 + 59);
  for (int i = 0; i < 60; ++i) c.tick();  // one more minute past 23:59
  TEST_ASSERT_EQUAL_UINT16(0, c.minutesOfDay());
}

void test_set_minutes_of_day_is_read_back_directly() {
  Clock c;
  c.setMinutesOfDay(8 * 60 + 15);
  TEST_ASSERT_EQUAL_UINT16(8 * 60 + 15, c.minutesOfDay());
}

void test_timer_starts_not_running() {
  Timer t;
  TEST_ASSERT_FALSE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(0, t.remaining());
}

void test_timer_start_counts_down_to_zero_then_stops() {
  Timer t;
  t.start(3);
  TEST_ASSERT_TRUE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(3, t.remaining());

  t.tick();
  t.tick();
  TEST_ASSERT_TRUE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(1, t.remaining());

  t.tick();
  TEST_ASSERT_FALSE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(0, t.remaining());
}

void test_timer_start_at_zero_is_immediately_expired() {
  Timer t;
  t.start(0);
  TEST_ASSERT_FALSE(t.isRunning());
}

void test_timer_tick_after_expiry_is_a_no_op() {
  Timer t;
  t.start(1);
  t.tick();
  TEST_ASSERT_FALSE(t.isRunning());
  t.tick();
  t.tick();
  TEST_ASSERT_FALSE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(0, t.remaining());
}

void test_timer_cancel_stops_and_zeroes_it() {
  Timer t;
  t.start(10);
  t.tick();
  t.cancel();
  TEST_ASSERT_FALSE(t.isRunning());
  TEST_ASSERT_EQUAL_UINT16(0, t.remaining());
}

void test_next_entered_minutes_shifts_digits() {
  uint16_t digits = 0;
  digits = nextEnteredMinutes(digits, 8);
  digits = nextEnteredMinutes(digits, 1);
  digits = nextEnteredMinutes(digits, 5);  // "815" = 8:15
  TEST_ASSERT_EQUAL_UINT16(8 * 60 + 15, decodeEnteredMinutes(digits));
}

void test_next_entered_minutes_hours_clamp_to_23() {
  uint16_t digits = 0;
  digits = nextEnteredMinutes(digits, 2);
  digits = nextEnteredMinutes(digits, 4);
  digits = nextEnteredMinutes(digits, 0);
  digits = nextEnteredMinutes(digits, 0);  // "2400" -> 24 hours -> clamps to 23:00
  TEST_ASSERT_EQUAL_UINT16(23 * 60, decodeEnteredMinutes(digits));
}

void test_next_entered_minutes_minutes_clamp_to_59() {
  uint16_t digits = 0;
  digits = nextEnteredMinutes(digits, 1);
  digits = nextEnteredMinutes(digits, 9);
  digits = nextEnteredMinutes(digits, 9);
  digits = nextEnteredMinutes(digits, 9);  // "1999" -> 19 hours, 99 minutes -> clamps to 19:59
  TEST_ASSERT_EQUAL_UINT16(19 * 60 + 59, decodeEnteredMinutes(digits));
}

void test_decode_does_not_clamp_valid_digits() {
  TEST_ASSERT_EQUAL_UINT16(0, decodeEnteredMinutes(0));
}

void test_split_digits_separates_high_and_low_fields() {
  DigitFields fields = splitDigits(1930);
  TEST_ASSERT_EQUAL_UINT8(19, fields.high);
  TEST_ASSERT_EQUAL_UINT8(30, fields.low);
}

void test_is_valid_time_h24_accepts_full_range() {
  TEST_ASSERT_TRUE(isValidTime(0, 0, TimeFormat::H24));
  TEST_ASSERT_TRUE(isValidTime(23, 59, TimeFormat::H24));
}

void test_is_valid_time_h24_rejects_out_of_range() {
  TEST_ASSERT_FALSE(isValidTime(24, 0, TimeFormat::H24));
  TEST_ASSERT_FALSE(isValidTime(0, 60, TimeFormat::H24));
}

void test_is_valid_time_h12_rejects_hour_zero() {
  TEST_ASSERT_FALSE(isValidTime(0, 30, TimeFormat::H12));  // a 12-hour face has no "0"
}

void test_is_valid_time_h12_accepts_one_through_twelve() {
  TEST_ASSERT_TRUE(isValidTime(1, 0, TimeFormat::H12));
  TEST_ASSERT_TRUE(isValidTime(12, 59, TimeFormat::H12));
}

void test_is_valid_time_h12_rejects_above_twelve() {
  TEST_ASSERT_FALSE(isValidTime(13, 0, TimeFormat::H12));
}

void test_to12_hour_maps_midnight_and_noon_to_twelve() {
  TEST_ASSERT_EQUAL_UINT8(12, to12Hour(0));
  TEST_ASSERT_EQUAL_UINT8(12, to12Hour(12));
}

void test_to12_hour_leaves_morning_hours_unchanged() {
  TEST_ASSERT_EQUAL_UINT8(1, to12Hour(1));
  TEST_ASSERT_EQUAL_UINT8(11, to12Hour(11));
}

void test_to12_hour_maps_afternoon_hours_down() {
  TEST_ASSERT_EQUAL_UINT8(1, to12Hour(13));
  TEST_ASSERT_EQUAL_UINT8(11, to12Hour(23));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_at_midnight);
  RUN_TEST(test_tick_advances_one_minute_after_sixty_ticks);
  RUN_TEST(test_tick_wraps_at_midnight);
  RUN_TEST(test_set_minutes_of_day_is_read_back_directly);
  RUN_TEST(test_timer_starts_not_running);
  RUN_TEST(test_timer_start_counts_down_to_zero_then_stops);
  RUN_TEST(test_timer_start_at_zero_is_immediately_expired);
  RUN_TEST(test_timer_tick_after_expiry_is_a_no_op);
  RUN_TEST(test_timer_cancel_stops_and_zeroes_it);
  RUN_TEST(test_next_entered_minutes_shifts_digits);
  RUN_TEST(test_next_entered_minutes_hours_clamp_to_23);
  RUN_TEST(test_next_entered_minutes_minutes_clamp_to_59);
  RUN_TEST(test_decode_does_not_clamp_valid_digits);
  RUN_TEST(test_split_digits_separates_high_and_low_fields);
  RUN_TEST(test_is_valid_time_h24_accepts_full_range);
  RUN_TEST(test_is_valid_time_h24_rejects_out_of_range);
  RUN_TEST(test_is_valid_time_h12_rejects_hour_zero);
  RUN_TEST(test_is_valid_time_h12_accepts_one_through_twelve);
  RUN_TEST(test_is_valid_time_h12_rejects_above_twelve);
  RUN_TEST(test_to12_hour_maps_midnight_and_noon_to_twelve);
  RUN_TEST(test_to12_hour_leaves_morning_hours_unchanged);
  RUN_TEST(test_to12_hour_maps_afternoon_hours_down);
  return UNITY_END();
}
