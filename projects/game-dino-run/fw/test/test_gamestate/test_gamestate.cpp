#include <unity.h>

#include "GameState.h"

using namespace gamestate;

void setUp() {}
void tearDown() {}

void test_press_from_start_begins_playing() {
  TEST_ASSERT_EQUAL(static_cast<int>(State::Playing), static_cast<int>(next(State::Start)));
}

void test_press_while_playing_is_a_noop() {
  TEST_ASSERT_EQUAL(static_cast<int>(State::Playing), static_cast<int>(next(State::Playing)));
}

void test_press_from_end_returns_to_start() {
  TEST_ASSERT_EQUAL(static_cast<int>(State::Start), static_cast<int>(next(State::End)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_press_from_start_begins_playing);
  RUN_TEST(test_press_while_playing_is_a_noop);
  RUN_TEST(test_press_from_end_returns_to_start);
  return UNITY_END();
}
