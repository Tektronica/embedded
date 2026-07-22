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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_at_midnight);
  RUN_TEST(test_tick_advances_one_minute_after_sixty_ticks);
  RUN_TEST(test_tick_wraps_at_midnight);
  RUN_TEST(test_set_minutes_of_day_is_read_back_directly);
  RUN_TEST(test_next_entered_minutes_shifts_digits);
  RUN_TEST(test_next_entered_minutes_hours_clamp_to_23);
  RUN_TEST(test_next_entered_minutes_minutes_clamp_to_59);
  RUN_TEST(test_decode_does_not_clamp_valid_digits);
  return UNITY_END();
}
