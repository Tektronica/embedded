#include <unity.h>

#include "Cycle.h"

using namespace cycle;

void setUp() {}
void tearDown() {}

void test_cycles_through_all_phases_back_to_surface() {
  Phase p = Phase::Surface;
  p = next(p);
  TEST_ASSERT_EQUAL(static_cast<int>(Phase::Descent), static_cast<int>(p));
  p = next(p);
  TEST_ASSERT_EQUAL(static_cast<int>(Phase::Park), static_cast<int>(p));
  p = next(p);
  TEST_ASSERT_EQUAL(static_cast<int>(Phase::Ascent), static_cast<int>(p));
  p = next(p);
  TEST_ASSERT_EQUAL(static_cast<int>(Phase::Surface), static_cast<int>(p));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_cycles_through_all_phases_back_to_surface);
  return UNITY_END();
}
