#include <unity.h>

#include "Microwave.h"

using namespace microwave;

void setUp() {}
void tearDown() {}

void test_starts_idle() {
  Controller c;
  TEST_ASSERT_TRUE(c.state() == State::Idle);
}

void test_digit_from_idle_enters_setting() {
  Controller c;
  c.handle(Event{EventType::Digit, 3});
  TEST_ASSERT_TRUE(c.state() == State::Setting);
  TEST_ASSERT_EQUAL_UINT16(3, c.displaySeconds());  // "3" alone = 0:03
}

void test_entering_300_reads_as_three_minutes() {
  Controller c;
  c.handle(Event{EventType::Digit, 3});
  c.handle(Event{EventType::Digit, 0});
  c.handle(Event{EventType::Digit, 0});
  TEST_ASSERT_EQUAL_UINT16(3 * 60, c.displaySeconds());  // "300" = 3:00
}

void test_seconds_above_59_clamp_instead_of_carrying() {
  Controller c;
  c.handle(Event{EventType::Digit, 9});
  c.handle(Event{EventType::Digit, 9});  // "99" -> 0 min, 99 sec -> clamps to 59 sec
  TEST_ASSERT_EQUAL_UINT16(59, c.displaySeconds());
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
  TEST_ASSERT_EQUAL_UINT16(5, c.displaySeconds());

  for (int i = 0; i < 4; ++i) c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Running);
  TEST_ASSERT_EQUAL_UINT16(1, c.displaySeconds());

  c.handle(Event{EventType::Tick, 0});
  TEST_ASSERT_TRUE(c.state() == State::Done);
  TEST_ASSERT_EQUAL_UINT16(0, c.displaySeconds());
}

void test_cancel_from_running_resets_to_idle() {
  Controller c;
  c.handle(Event{EventType::Digit, 5});
  c.handle(Event{EventType::Start, 0});
  c.handle(Event{EventType::Cancel, 0});
  TEST_ASSERT_TRUE(c.state() == State::Idle);
  TEST_ASSERT_EQUAL_UINT16(0, c.displaySeconds());
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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_idle);
  RUN_TEST(test_digit_from_idle_enters_setting);
  RUN_TEST(test_entering_300_reads_as_three_minutes);
  RUN_TEST(test_seconds_above_59_clamp_instead_of_carrying);
  RUN_TEST(test_start_with_no_time_entered_is_ignored);
  RUN_TEST(test_start_after_entering_time_runs_and_counts_down);
  RUN_TEST(test_cancel_from_running_resets_to_idle);
  RUN_TEST(test_any_key_from_done_resets_to_idle);
  return UNITY_END();
}
