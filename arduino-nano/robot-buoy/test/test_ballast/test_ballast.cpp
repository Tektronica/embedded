#include <unity.h>

#include "Ballast.h"

using namespace ballast;

void setUp() {}
void tearDown() {}

void test_stroke_steps_matches_microsteps_times_revolutions() {
  long expected = static_cast<long>(MICROSTEPS) * STEPS_PER_REV * FILL_REVOLUTIONS;
  TEST_ASSERT_EQUAL(expected, strokeSteps());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_stroke_steps_matches_microsteps_times_revolutions);
  return UNITY_END();
}
