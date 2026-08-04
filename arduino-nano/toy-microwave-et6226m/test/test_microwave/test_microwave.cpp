#include <unity.h>

#include "Microwave.h"

using namespace microwave;

void setUp() {}
void tearDown() {}

void test_starts_idle() {
  Controller c;
  TEST_ASSERT_TRUE(c.state() == State::Idle);
}

void test_idle_shows_clock_starting_at_midnight() {
  Controller c;
  // Midnight displays as 12:00, not 0:00 -- Idle's display is 12-hour by default (a 12-hour face
  // has no "0"), even though no RTC seeds a real time here.
  TEST_ASSERT_EQUAL_UINT16(12 * 60, c.displayValue());
}

void test_clock_advances_once_per_tick_while_idle() {
  Controller c;
  for (int i = 0; i < 90; ++i) c.handle(Event{EventType::Tick, 0});  // 90 seconds
  TEST_ASSERT_EQUAL_UINT16(12 * 60 + 1, c.displayValue());  // 12:01, 1 minute past midnight
}

void test_clock_keeps_ticking_while_cooking() {
  Controller c;
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});  // Running, 0:05 remaining
  for (int i = 0; i < 120; ++i) c.handle(Event{EventType::Tick, 0});  // 2 minutes, well past Done

  TEST_ASSERT_TRUE(c.state() == State::Done);
  c.handle(Event{EventType::Cancel, 0});  // back to Idle to read the clock
  TEST_ASSERT_EQUAL_UINT16(12 * 60 + 2, c.displayValue());  // 12:02, the clock kept advancing
}

void test_digit_from_idle_enters_setting() {
  Controller c;
  c.handle(Event{EventType::Digit, 3});
  TEST_ASSERT_TRUE(c.state() == State::Setting);
  TEST_ASSERT_EQUAL_UINT16(3, c.displayValue());  // "3" alone = 0:03
}

void test_entering_300_reads_as_three_minutes() {
  Controller c;
  c.handle(Event{EventType::Digit, 3});
  c.handle(Event{EventType::Digit, 0});
  c.handle(Event{EventType::Digit, 0});
  TEST_ASSERT_EQUAL_UINT16(3 * 60, c.displayValue());  // "300" = 3:00
}

void test_seconds_above_59_clamp_instead_of_carrying() {
  Controller c;
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 9});  // "99" -> 0 min, 99 sec -> clamps to 59 sec
  TEST_ASSERT_EQUAL_UINT16(59, c.displayValue());
}

void test_start_with_no_time_entered_is_ignored() {
  Controller c;
  c.handle(Event{EventType::Start, 0});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
}

void test_start_after_entering_time_runs_and_counts_down() {
  Controller c;
  c.handle(Event{EventType::Digit, 5});  // 0:05
  c.handle(Event{EventType::Start, 0});
  TEST_ASSERT_TRUE(c.state() == State::Running);
  TEST_ASSERT_EQUAL_UINT16(5, c.displayValue());

  for (int i = 0; i < 4; ++i) c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Running);
  TEST_ASSERT_EQUAL_UINT16(1, c.displayValue());

  c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Done);
  TEST_ASSERT_EQUAL_UINT16(0, c.displayValue());
}

void test_cancel_from_running_resets_to_idle() {
  Controller c;
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  c.handle(Event{EventType::Cancel, 0});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_EQUAL_UINT16(12 * 60, c.displayValue());  // 12:00, clock never ticked (no Tick here)
}

void test_cook_time_is_cooking_while_running() {
  Controller c;
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  TEST_ASSERT_TRUE(c.isCooking());
}

void test_timer_key_from_idle_enters_setting_as_a_kitchen_timer() {
  Controller c;
  c.handle(Event{EventType::Timer, 0});
  TEST_ASSERT_TRUE(c.state() == State::Setting);
  TEST_ASSERT_EQUAL_UINT16(0, c.displayValue());  // starts blank, unlike Digit which seeds a digit
}

void test_timer_counts_down_like_cook_time_but_is_not_cooking() {
  Controller c;
  c.handle(Event{EventType::Timer, 0});
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});

  TEST_ASSERT_TRUE(c.state() == State::Running);
  TEST_ASSERT_FALSE(c.isCooking());  // a kitchen timer never turns on the motor/fan/light
  TEST_ASSERT_EQUAL_UINT16(5, c.displayValue());

  for (int i = 0; i < 5; ++i) c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Done);
  TEST_ASSERT_FALSE(c.isCooking());
}

void test_cancel_from_timer_setting_clears_it_like_cook_time() {
  Controller c;
  c.handle(Event{EventType::Timer, 0});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Cancel, 0});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_EQUAL_UINT16(12 * 60, c.displayValue());  // 12:00, clock never ticked (no Tick here)
}

void test_cancel_from_running_timer_ends_it() {
  Controller c;
  c.handle(Event{EventType::Timer, 0});
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  c.handle(Event{EventType::Cancel, 0});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_FALSE(c.isCooking());
}

void test_digit_from_idle_after_a_timer_is_cooking_again() {
  // A later cook-time entry must not be mistaken for a leftover kitchen timer.
  Controller c;
  c.handle(Event{EventType::Timer, 0});
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  c.handle(Event{EventType::Cancel, 0});

  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  TEST_ASSERT_TRUE(c.isCooking());
}

void test_any_key_from_done_resets_to_idle() {
  Controller c;
  c.handle(Event{EventType::Digit, 1});
  c.handle(Event{EventType::Start, 0});
  c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Done);

  c.handle(Event{EventType::Digit, 7});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
}

void test_clock_key_from_idle_enters_clock_set() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  TEST_ASSERT_TRUE(c.state() == State::ClockSet);
  TEST_ASSERT_EQUAL_UINT16(0, c.displayValue());
}

void test_entering_1930_in_clock_set_reads_as_seven_thirty_pm() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  c.handle(Event{EventType::Digit, 1});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 3});
  c.handle(Event{EventType::Digit, 0});
  TEST_ASSERT_EQUAL_UINT16(19 * 60 + 30, c.displayValue());  // "1930" = 19:30
}

void test_clock_set_preview_clamps_hours_to_23_while_typing() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  c.handle(Event{EventType::Digit, 2});
  c.handle(Event{EventType::Digit, 4});
  c.handle(Event{EventType::Digit, 0});
  c.handle(Event{EventType::Digit, 0});  // "2400" -> 24 hours -> live preview clamps to 23:00
  TEST_ASSERT_EQUAL_UINT16(23 * 60, c.displayValue());
}

void test_clock_set_rejects_out_of_range_hour_on_start() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  c.handle(Event{EventType::Digit, 2});
  c.handle(Event{EventType::Digit, 4});
  c.handle(Event{EventType::Digit, 0});
  c.handle(Event{EventType::Digit, 0});  // "2400" -> 24 hours, invalid
  c.handle(Event{EventType::Start, 0});  // rejected -- stays in ClockSet, nothing committed

  TEST_ASSERT_TRUE(c.state() == State::ClockSet);
}

void test_start_confirms_clock_set_and_returns_to_idle() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  c.handle(Event{EventType::Digit, 8});
  c.handle(Event{EventType::Digit, 1});
  c.handle(Event{EventType::Digit, 5});  // "815" = 8:15
  c.handle(Event{EventType::Start, 0});

  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_EQUAL_UINT16(8 * 60 + 15, c.displayValue());
}

void test_cancel_discards_clock_set() {
  Controller c;
  c.handle(Event{EventType::Digit, 1});
  c.handle(Event{EventType::Start, 0});
  for (int i = 0; i < 60; ++i) c.handle(Event{EventType::Tick, 0});
  c.handle(Event{EventType::Cancel, 0});  // back to Idle; clock should read 0:01 by now

  c.handle(Event{EventType::Clock, 0});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Cancel, 0});  // discard the "99:99" entry

  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_EQUAL_UINT16(12 * 60 + 1, c.displayValue());  // 12:01, unchanged from before ClockSet
}

void test_clock_does_not_advance_while_being_set() {
  Controller c;
  c.handle(Event{EventType::Clock, 0});
  for (int i = 0; i < 200; ++i) c.handle(Event{EventType::Tick, 0});  // Ticks ignored in ClockSet
  c.handle(Event{EventType::Cancel, 0});
  TEST_ASSERT_EQUAL_UINT16(12 * 60, c.displayValue());  // 12:00, clock never moved
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_idle);
  RUN_TEST(test_idle_shows_clock_starting_at_midnight);
  RUN_TEST(test_clock_advances_once_per_tick_while_idle);
  RUN_TEST(test_clock_keeps_ticking_while_cooking);
  RUN_TEST(test_digit_from_idle_enters_setting);
  RUN_TEST(test_entering_300_reads_as_three_minutes);
  RUN_TEST(test_seconds_above_59_clamp_instead_of_carrying);
  RUN_TEST(test_start_with_no_time_entered_is_ignored);
  RUN_TEST(test_start_after_entering_time_runs_and_counts_down);
  RUN_TEST(test_cancel_from_running_resets_to_idle);
  RUN_TEST(test_cook_time_is_cooking_while_running);
  RUN_TEST(test_timer_key_from_idle_enters_setting_as_a_kitchen_timer);
  RUN_TEST(test_timer_counts_down_like_cook_time_but_is_not_cooking);
  RUN_TEST(test_cancel_from_timer_setting_clears_it_like_cook_time);
  RUN_TEST(test_cancel_from_running_timer_ends_it);
  RUN_TEST(test_digit_from_idle_after_a_timer_is_cooking_again);
  RUN_TEST(test_any_key_from_done_resets_to_idle);
  RUN_TEST(test_clock_key_from_idle_enters_clock_set);
  RUN_TEST(test_entering_1930_in_clock_set_reads_as_seven_thirty_pm);
  RUN_TEST(test_clock_set_preview_clamps_hours_to_23_while_typing);
  RUN_TEST(test_clock_set_rejects_out_of_range_hour_on_start);
  RUN_TEST(test_start_confirms_clock_set_and_returns_to_idle);
  RUN_TEST(test_cancel_discards_clock_set);
  RUN_TEST(test_clock_does_not_advance_while_being_set);
  return UNITY_END();
}
