#include <unity.h>

#include "Buzzer.h"

using namespace buzzer;

void setUp() {}
void tearDown() {}

void test_none_is_always_off() {
  TEST_ASSERT_FALSE(toneStateFor(Pattern::None, 0).on);
}

void test_keypress_turns_off_after_its_duration() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::KeyPress, 0).on);
  TEST_ASSERT_TRUE(toneStateFor(Pattern::KeyPress, KEYPRESS_MS - 1).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::KeyPress, KEYPRESS_MS).on);
}

void test_done_plays_four_beeps_with_gaps() {
  uint16_t cycleMs = DONE_BEEP_MS + DONE_BEEP_GAP_MS;
  for (uint8_t i = 0; i < DONE_BEEP_COUNT; ++i) {
    uint32_t beepStart = static_cast<uint32_t>(i) * cycleMs;
    TEST_ASSERT_TRUE(toneStateFor(Pattern::Done, beepStart).on);
    TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, beepStart + DONE_BEEP_MS).on);  // gap
  }
  uint32_t totalMs = static_cast<uint32_t>(DONE_BEEP_COUNT) * cycleMs;
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, totalMs).on);  // sequence over
}

void test_error_turns_off_after_its_duration() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Error, 0).on);
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Error, ERROR_MS - 1).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Error, ERROR_MS).on);
}

void test_is_finished_for_none() {
  TEST_ASSERT_TRUE(isFinished(Pattern::None, 0));
}

void test_is_finished_for_done_waits_for_the_whole_sequence() {
  // Mid-sequence, Done is off during a gap (per test_done_plays_four_beeps_with_gaps) but must
  // NOT be reported as finished yet -- this is the bug isFinished() exists to avoid.
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, DONE_BEEP_MS).on);
  TEST_ASSERT_FALSE(isFinished(Pattern::Done, DONE_BEEP_MS));

  uint32_t totalMs = static_cast<uint32_t>(DONE_BEEP_COUNT) * (DONE_BEEP_MS + DONE_BEEP_GAP_MS);
  TEST_ASSERT_TRUE(isFinished(Pattern::Done, totalMs));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_none_is_always_off);
  RUN_TEST(test_keypress_turns_off_after_its_duration);
  RUN_TEST(test_done_plays_four_beeps_with_gaps);
  RUN_TEST(test_error_turns_off_after_its_duration);
  RUN_TEST(test_is_finished_for_none);
  RUN_TEST(test_is_finished_for_done_waits_for_the_whole_sequence);
  return UNITY_END();
}
