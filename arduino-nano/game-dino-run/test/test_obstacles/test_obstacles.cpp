#include <unity.h>

#include "Obstacles.h"

using namespace obstacles;

void setUp() {}
void tearDown() {}

void test_advance_without_recycling_moves_both_by_speed() {
  State s;  // front = {OneTall, 128}, next = {OneSmall, 200}
  advance(s, 8, 100, Shape::TwoTall);
  TEST_ASSERT_EQUAL_INT16(120, s.front.x);
  TEST_ASSERT_EQUAL(static_cast<int>(Shape::OneTall), static_cast<int>(s.front.shape));
  TEST_ASSERT_EQUAL_INT16(192, s.next.x);
  TEST_ASSERT_EQUAL(static_cast<int>(Shape::OneSmall), static_cast<int>(s.next.shape));
}

void test_advance_recycles_front_once_it_scrolls_offscreen() {
  State s;
  s.front = Pair{Shape::OneTall, -15};
  s.next = Pair{Shape::TwoTall, 50};

  advance(s, 8, 40, Shape::ThreeSmall);  // -15 - 8 = -23, past OFFSCREEN_X (-20)

  // front adopts next's shape/position unchanged this frame (matches the original)
  TEST_ASSERT_EQUAL(static_cast<int>(Shape::TwoTall), static_cast<int>(s.front.shape));
  TEST_ASSERT_EQUAL_INT16(50, s.front.x);

  // next is freshly spawned at the recycled position plus the injected random gap
  TEST_ASSERT_EQUAL(static_cast<int>(Shape::ThreeSmall), static_cast<int>(s.next.shape));
  TEST_ASSERT_EQUAL_INT16(90, s.next.x);
}

void test_collides_when_obstacle_overlaps_and_dino_too_low_to_clear() {
  TEST_ASSERT_TRUE(collides(Shape::OneTall, 0, 0));  // sizeOf(OneTall) = {10, 20}
}

void test_no_collision_once_obstacle_fully_passed() {
  TEST_ASSERT_FALSE(collides(Shape::OneTall, 16, 0));   // past the left edge
  TEST_ASSERT_FALSE(collides(Shape::OneTall, -6, 0));   // trailing edge (x+width=4) below 5
}

void test_no_collision_when_dino_jumps_high_enough_to_clear() {
  TEST_ASSERT_FALSE(collides(Shape::OneTall, 0, 18));  // height 20 - 3 = 17 is the max that still hits
  TEST_ASSERT_TRUE(collides(Shape::OneTall, 0, 17));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_advance_without_recycling_moves_both_by_speed);
  RUN_TEST(test_advance_recycles_front_once_it_scrolls_offscreen);
  RUN_TEST(test_collides_when_obstacle_overlaps_and_dino_too_low_to_clear);
  RUN_TEST(test_no_collision_once_obstacle_fully_passed);
  RUN_TEST(test_no_collision_when_dino_jumps_high_enough_to_clear);
  return UNITY_END();
}
