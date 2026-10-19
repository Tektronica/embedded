#include <unity.h>

#include "Dino.h"

using namespace dino;

void setUp() {}
void tearDown() {}

void test_leg_frame_cycles_while_grounded() {
  State s;
  TEST_ASSERT_EQUAL_UINT8(0, s.legFrame);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(1, s.legFrame);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(2, s.legFrame);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(0, s.legFrame);
}

void test_start_jump_from_grounded_returns_true_and_ascends() {
  State s;
  TEST_ASSERT_TRUE(startJump(s));
  TEST_ASSERT_EQUAL(static_cast<int>(JumpPhase::Ascending), static_cast<int>(s.phase));
}

void test_start_jump_while_airborne_is_ignored() {
  State s;
  startJump(s);
  TEST_ASSERT_FALSE(startJump(s));
}

void test_jump_arc_ascends_then_descends_then_lands() {
  State s;
  startJump(s);

  advance(s);
  TEST_ASSERT_EQUAL_UINT8(8, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(16, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(24, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(32, s.jumpHeight);
  TEST_ASSERT_EQUAL(static_cast<int>(JumpPhase::Ascending), static_cast<int>(s.phase));

  advance(s);  // 40 > JUMP_PEAK -- switches to descending this same call
  TEST_ASSERT_EQUAL_UINT8(40, s.jumpHeight);
  TEST_ASSERT_EQUAL(static_cast<int>(JumpPhase::Descending), static_cast<int>(s.phase));

  advance(s);
  TEST_ASSERT_EQUAL_UINT8(32, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(24, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(16, s.jumpHeight);
  advance(s);
  TEST_ASSERT_EQUAL_UINT8(8, s.jumpHeight);
  TEST_ASSERT_EQUAL(static_cast<int>(JumpPhase::Descending), static_cast<int>(s.phase));

  advance(s);  // 0 < JUMP_LAND_THRESHOLD -- lands this same call
  TEST_ASSERT_EQUAL(static_cast<int>(JumpPhase::Grounded), static_cast<int>(s.phase));
  TEST_ASSERT_EQUAL_UINT8(0, s.jumpHeight);
}

void test_leg_frame_is_untouched_during_a_jump() {
  State s;
  advance(s);  // legFrame -> 1
  TEST_ASSERT_EQUAL_UINT8(1, s.legFrame);
  startJump(s);
  advance(s);  // airborne now -- legFrame must not change
  TEST_ASSERT_EQUAL_UINT8(1, s.legFrame);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_leg_frame_cycles_while_grounded);
  RUN_TEST(test_start_jump_from_grounded_returns_true_and_ascends);
  RUN_TEST(test_start_jump_while_airborne_is_ignored);
  RUN_TEST(test_jump_arc_ascends_then_descends_then_lands);
  RUN_TEST(test_leg_frame_is_untouched_during_a_jump);
  return UNITY_END();
}
