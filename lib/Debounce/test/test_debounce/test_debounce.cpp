#include <unity.h>

#include "Debounce.h"

using namespace debounce;

void setUp() {}
void tearDown() {}

struct Pos {
  uint8_t x;
  uint8_t y;
};
bool operator==(Pos a, Pos b) { return a.x == b.x && a.y == b.y; }

// --- Button ---

void test_button_fires_once_per_debounced_press() {
  Button b;
  int edges = 0;
  bool seq[] = {false, true, true, true, true, false, false, false};  // press (held), release
  for (bool s : seq)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(1, edges);
}

void test_button_ignores_bounce() {
  Button b;
  int edges = 0;
  bool bounce[] = {true, false, true, false, true, false};
  for (bool s : bounce)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(0, edges);
}

void test_button_uses_supplied_sample_count() {
  Button b(5);
  for (int i = 0; i < 4; ++i) TEST_ASSERT_FALSE(b.pressed(true));
  TEST_ASSERT_TRUE(b.pressed(true));
}

// --- Debouncer<T> ---

void test_commits_after_consecutive_samples() {
  Debouncer<uint8_t> d(0xFF);
  TEST_ASSERT_FALSE(d.update(5));
  TEST_ASSERT_FALSE(d.update(5));
  TEST_ASSERT_TRUE(d.update(5));
  TEST_ASSERT_EQUAL_UINT8(5, d.value());
}

void test_holding_the_committed_value_does_not_recommit() {
  Debouncer<uint8_t> d(0xFF);
  for (int i = 0; i < DEFAULT_SAMPLES; ++i) d.update(5);
  TEST_ASSERT_FALSE(d.update(5));
}

void test_changing_candidate_restarts_the_count() {
  Debouncer<uint8_t> d(0xFF);
  d.update(5);
  d.update(5);
  TEST_ASSERT_FALSE(d.update(6));  // a different reading doesn't inherit 5's count
  TEST_ASSERT_FALSE(d.update(6));
  TEST_ASSERT_TRUE(d.update(6));
  TEST_ASSERT_EQUAL_UINT8(6, d.value());
}

void test_release_then_repress_commits_again() {
  Debouncer<uint8_t> d(0xFF);
  for (int i = 0; i < DEFAULT_SAMPLES; ++i) d.update(14);
  for (int i = 0; i < DEFAULT_SAMPLES; ++i) d.update(0xFF);
  TEST_ASSERT_EQUAL_UINT8(0xFF, d.value());
  bool committed = false;
  for (int i = 0; i < DEFAULT_SAMPLES; ++i) committed = d.update(14);
  TEST_ASSERT_TRUE(committed);
}

void test_struct_values() {
  Debouncer<Pos> d(Pos{0, 0});
  for (int i = 0; i < DEFAULT_SAMPLES - 1; ++i) TEST_ASSERT_FALSE(d.update(Pos{2, 3}));
  TEST_ASSERT_TRUE(d.update(Pos{2, 3}));
  TEST_ASSERT_EQUAL_UINT8(2, d.value().x);
  TEST_ASSERT_EQUAL_UINT8(3, d.value().y);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_button_fires_once_per_debounced_press);
  RUN_TEST(test_button_ignores_bounce);
  RUN_TEST(test_button_uses_supplied_sample_count);
  RUN_TEST(test_commits_after_consecutive_samples);
  RUN_TEST(test_holding_the_committed_value_does_not_recommit);
  RUN_TEST(test_changing_candidate_restarts_the_count);
  RUN_TEST(test_release_then_repress_commits_again);
  RUN_TEST(test_struct_values);
  return UNITY_END();
}
