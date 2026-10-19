#include <unity.h>

#include "Song.h"

using namespace song;

void setUp() {}
void tearDown() {}

void test_duration_ms_for_a_quarter_note() {
  TEST_ASSERT_EQUAL_UINT16(300, durationMs(4, 200));  // whole note = 1200ms at 200 BPM
}

void test_duration_ms_for_a_dotted_quarter_note() {
  TEST_ASSERT_EQUAL_UINT16(450, durationMs(-4, 200));  // 300ms + half again
}

void test_next_cycles_through_all_tracks_and_wraps() {
  TEST_ASSERT_EQUAL(Track::Tetris, next(Track::Scale));
  TEST_ASSERT_EQUAL(Track::Mario, next(Track::Tetris));
  TEST_ASSERT_EQUAL(Track::Doom, next(Track::Mario));
  TEST_ASSERT_EQUAL(Track::Nokia, next(Track::Doom));
  TEST_ASSERT_EQUAL(Track::Scale, next(Track::Nokia));
}

void test_scale_starts_on_c4() {
  ToneState t = toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 0);
  TEST_ASSERT_TRUE(t.on);
  TEST_ASSERT_EQUAL_UINT16(notes::C4, t.frequencyHz);
}

void test_scale_note_goes_silent_during_its_articulation_gap() {
  TEST_ASSERT_TRUE(toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 269).on);
  TEST_ASSERT_FALSE(toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 299).on);  // last 10%
}

void test_scale_advances_to_the_next_note() {
  ToneState t = toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 300);
  TEST_ASSERT_TRUE(t.on);
  TEST_ASSERT_EQUAL_UINT16(notes::D4, t.frequencyHz);
}

void test_scale_loops_back_to_the_start() {
  ToneState looped = toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 2400);  // 8 * 300ms
  ToneState start  = toneStateFor(SCALE, SCALE_COUNT, SCALE_TEMPO_BPM, 0);
  TEST_ASSERT_EQUAL_UINT16(start.frequencyHz, looped.frequencyHz);
  TEST_ASSERT_EQUAL(start.on, looped.on);
}

void test_rest_is_always_off() {
  Note rest[] = {{notes::REST, 4}};
  TEST_ASSERT_FALSE(toneStateFor(rest, 1, 200, 0).on);
}

void test_duration_ms_for_an_invalid_zero_divider_is_zero() {
  TEST_ASSERT_EQUAL_UINT16(0, durationMs(0, 200));
}

void test_tone_state_for_an_empty_track_is_off() {
  TEST_ASSERT_FALSE(toneStateFor(SCALE, 0, SCALE_TEMPO_BPM, 0).on);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_duration_ms_for_a_quarter_note);
  RUN_TEST(test_duration_ms_for_a_dotted_quarter_note);
  RUN_TEST(test_next_cycles_through_all_tracks_and_wraps);
  RUN_TEST(test_scale_starts_on_c4);
  RUN_TEST(test_scale_note_goes_silent_during_its_articulation_gap);
  RUN_TEST(test_scale_advances_to_the_next_note);
  RUN_TEST(test_scale_loops_back_to_the_start);
  RUN_TEST(test_rest_is_always_off);
  RUN_TEST(test_duration_ms_for_an_invalid_zero_divider_is_zero);
  RUN_TEST(test_tone_state_for_an_empty_track_is_off);
  return UNITY_END();
}
