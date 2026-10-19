#include <unity.h>

#include "Buzzer.h"

using namespace buzzer;

void setUp() {}
void tearDown() {}

void test_none_is_always_off() {
  TEST_ASSERT_FALSE(toneStateFor(Pattern::None, 0).on);
}

void test_jump_turns_off_after_its_duration() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Jump, 0).on);
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Jump, JUMP_MS - 1).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Jump, JUMP_MS).on);
}

void test_milestone_plays_two_tones_with_a_silent_gap() {
  ToneState t0 = toneStateFor(Pattern::Milestone, 0);
  TEST_ASSERT_TRUE(t0.on);
  TEST_ASSERT_EQUAL_UINT16(MILESTONE_TONE1_HZ, t0.frequencyHz);

  TEST_ASSERT_FALSE(toneStateFor(Pattern::Milestone, MILESTONE_TONE_MS).on);  // gap

  ToneState t1 = toneStateFor(Pattern::Milestone, MILESTONE_GAP_MS);
  TEST_ASSERT_TRUE(t1.on);
  TEST_ASSERT_EQUAL_UINT16(MILESTONE_TONE2_HZ, t1.frequencyHz);

  TEST_ASSERT_FALSE(toneStateFor(Pattern::Milestone, MILESTONE_GAP_MS + MILESTONE_TONE_MS).on);
}

void test_hit_plays_two_tones_with_a_silent_gap() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Hit, 0).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Hit, HIT_TONE_MS).on);  // gap
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Hit, HIT_GAP_MS).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Hit, HIT_GAP_MS + HIT_TONE_MS).on);
}

void test_is_finished_for_none() {
  TEST_ASSERT_TRUE(isFinished(Pattern::None, 0));
}

void test_is_finished_for_milestone_waits_for_the_whole_sequence() {
  // Mid-sequence, Milestone is off during its gap (per test_milestone_plays_two_tones...) but
  // must NOT be reported as finished yet -- this is the bug isFinished() exists to avoid.
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Milestone, MILESTONE_TONE_MS).on);
  TEST_ASSERT_FALSE(isFinished(Pattern::Milestone, MILESTONE_TONE_MS));

  TEST_ASSERT_TRUE(isFinished(Pattern::Milestone, MILESTONE_GAP_MS + MILESTONE_TONE_MS));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_none_is_always_off);
  RUN_TEST(test_jump_turns_off_after_its_duration);
  RUN_TEST(test_milestone_plays_two_tones_with_a_silent_gap);
  RUN_TEST(test_hit_plays_two_tones_with_a_silent_gap);
  RUN_TEST(test_is_finished_for_none);
  RUN_TEST(test_is_finished_for_milestone_waits_for_the_whole_sequence);
  return UNITY_END();
}
