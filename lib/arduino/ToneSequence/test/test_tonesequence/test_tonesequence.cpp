#include <unity.h>

#include "ToneSequence.h"

using namespace tonesequence;

void setUp() {}
void tearDown() {}

constexpr Step BEEP_GAP_BEEP[] = {{1000, 100}, {SILENCE, 50}, {1250, 100}};
constexpr Sequence ONE_SHOT{BEEP_GAP_BEEP, 3, false};
constexpr Sequence LOOPED{BEEP_GAP_BEEP, 3, true};
constexpr Sequence EMPTY{nullptr, 0, false};

void test_total_is_the_sum_of_steps() { TEST_ASSERT_EQUAL_UINT32(250, totalMs(ONE_SHOT)); }

void test_plays_each_step_in_order() {
  ToneState first = toneStateAt(ONE_SHOT, 0);
  TEST_ASSERT_TRUE(first.on);
  TEST_ASSERT_EQUAL_UINT16(1000, first.frequencyHz);
  TEST_ASSERT_TRUE(toneStateAt(ONE_SHOT, 99).on);
  TEST_ASSERT_FALSE(toneStateAt(ONE_SHOT, 100).on);  // gap
  ToneState second = toneStateAt(ONE_SHOT, 150);
  TEST_ASSERT_TRUE(second.on);
  TEST_ASSERT_EQUAL_UINT16(1250, second.frequencyHz);
}

void test_one_shot_goes_silent_and_finishes() {
  TEST_ASSERT_FALSE(isFinished(ONE_SHOT, 100));  // silent mid-sequence is not finished
  TEST_ASSERT_FALSE(toneStateAt(ONE_SHOT, 250).on);
  TEST_ASSERT_TRUE(isFinished(ONE_SHOT, 250));
  TEST_ASSERT_FALSE(toneStateAt(ONE_SHOT, 99999).on);
}

void test_looped_repeats_and_never_finishes() {
  ToneState again = toneStateAt(LOOPED, 250);
  TEST_ASSERT_TRUE(again.on);
  TEST_ASSERT_EQUAL_UINT16(1000, again.frequencyHz);
  TEST_ASSERT_EQUAL_UINT16(1250, toneStateAt(LOOPED, 250 + 150).frequencyHz);
  TEST_ASSERT_FALSE(isFinished(LOOPED, 999999));
}

void test_empty_is_silent_and_finished() {
  TEST_ASSERT_FALSE(toneStateAt(EMPTY, 0).on);
  TEST_ASSERT_TRUE(isFinished(EMPTY, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_total_is_the_sum_of_steps);
  RUN_TEST(test_plays_each_step_in_order);
  RUN_TEST(test_one_shot_goes_silent_and_finishes);
  RUN_TEST(test_looped_repeats_and_never_finishes);
  RUN_TEST(test_empty_is_silent_and_finished);
  return UNITY_END();
}
