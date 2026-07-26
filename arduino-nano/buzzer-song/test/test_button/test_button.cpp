#include <unity.h>

#include "Button.h"

using namespace input;

void setUp() {}
void tearDown() {}

void test_button_fires_once_per_debounced_press() {
  Button b;
  int edges = 0;
  bool seq[] = {false, true, true, true, true, false, false, false};  // press (held), release
  for (bool s : seq)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(1, edges);  // exactly one press detected
}

void test_button_ignores_bounce() {
  Button b;
  int edges = 0;
  bool bounce[] = {true, false, true, false, true, false};  // never DEBOUNCE-consistent
  for (bool s : bounce)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(0, edges);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_button_fires_once_per_debounced_press);
  RUN_TEST(test_button_ignores_bounce);
  return UNITY_END();
}
