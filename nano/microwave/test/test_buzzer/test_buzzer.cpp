#include <unity.h>

#include "Buzzer.h"

using namespace buzzer;

void setUp() {}
void tearDown() {}

void test_none_is_always_off() {
  ToneState t = toneStateFor(Pattern::None, 0);
  TEST_ASSERT_FALSE(t.on);
}

void test_hum_is_always_on_at_hum_frequency() {
  ToneState t0 = toneStateFor(Pattern::Hum, 0);
  ToneState tLater = toneStateFor(Pattern::Hum, 999999);
  TEST_ASSERT_TRUE(t0.on);
  TEST_ASSERT_TRUE(tLater.on);
  TEST_ASSERT_EQUAL_UINT16(HUM_HZ, t0.frequencyHz);
}

void test_keypress_turns_off_after_its_duration() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::KeyPress, 0).on);
  TEST_ASSERT_TRUE(toneStateFor(Pattern::KeyPress, KEYPRESS_MS - 1).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::KeyPress, KEYPRESS_MS).on);
}

void test_error_turns_off_after_its_duration() {
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Error, 0).on);
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Error, ERROR_MS).on);
}

void test_done_beeps_the_configured_count_then_stops() {
  uint16_t cycleMs = DONE_BEEP_MS + DONE_BEEP_GAP_MS;

  TEST_ASSERT_TRUE(toneStateFor(Pattern::Done, 0).on);                  // first beep on
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, DONE_BEEP_MS).on);      // first gap
  TEST_ASSERT_TRUE(toneStateFor(Pattern::Done, cycleMs).on);            // second beep on

  uint32_t totalMs = static_cast<uint32_t>(DONE_BEEP_COUNT) * cycleMs;
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, totalMs).on);           // done beeping
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, totalMs + 5000).on);
}

void test_is_finished_for_hum_and_none() {
  TEST_ASSERT_FALSE(isFinished(Pattern::Hum, 999999));  // Hum never finishes on its own
  TEST_ASSERT_TRUE(isFinished(Pattern::None, 0));
}

void test_is_finished_for_done_waits_for_the_whole_sequence() {
  uint16_t cycleMs = DONE_BEEP_MS + DONE_BEEP_GAP_MS;
  uint32_t totalMs = static_cast<uint32_t>(DONE_BEEP_COUNT) * cycleMs;

  // Mid-sequence, Done is off during a gap (per test_done_beeps_the_configured_count_then_stops)
  // but must NOT be reported as finished yet — this is the bug isFinished() exists to avoid.
  TEST_ASSERT_FALSE(toneStateFor(Pattern::Done, DONE_BEEP_MS).on);
  TEST_ASSERT_FALSE(isFinished(Pattern::Done, DONE_BEEP_MS));

  TEST_ASSERT_TRUE(isFinished(Pattern::Done, totalMs));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_none_is_always_off);
  RUN_TEST(test_hum_is_always_on_at_hum_frequency);
  RUN_TEST(test_keypress_turns_off_after_its_duration);
  RUN_TEST(test_error_turns_off_after_its_duration);
  RUN_TEST(test_done_beeps_the_configured_count_then_stops);
  RUN_TEST(test_is_finished_for_hum_and_none);
  RUN_TEST(test_is_finished_for_done_waits_for_the_whole_sequence);
  return UNITY_END();
}
