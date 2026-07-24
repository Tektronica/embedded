#pragma once

#include <stdint.h>

// Scrolling ground obstacles: two are always in flight (front + next), recycled with a random
// gap/shape once the front one scrolls off-screen, plus collision detection against the dino.
// Hardware-free -- randomness is injected as a parameter rather than called internally -- so it
// unit-tests off-device with deterministic inputs; main.cpp supplies real random(...) values.
namespace obstacles {

// Numbered to match the original bitmap asset set; values are otherwise arbitrary.
enum class Shape : uint8_t { OneTall = 1, TwoTall = 2, ThreeTall = 3,
                              OneSmall = 4, TwoSmall = 5, ThreeSmall = 6 };

constexpr int16_t OFFSCREEN_X = -20;  // once scrolled past this, the front obstacle recycles

// An obstacle's collision footprint.
struct Size { uint8_t width; uint8_t height; };

// Collision footprint for a shape -- deliberately a hair narrower than the drawn sprite for
// TwoTall/ThreeTall (drawn at 20 wide, hitbox 17), giving a small forgiveness margin.
inline Size sizeOf(Shape shape) {
  switch (shape) {
    case Shape::OneTall:    return Size{10, 20};
    case Shape::TwoTall:    return Size{17, 20};
    case Shape::ThreeTall:  return Size{17, 20};
    case Shape::OneSmall:   return Size{6, 12};
    case Shape::TwoSmall:   return Size{12, 12};
    case Shape::ThreeSmall: return Size{17, 12};
  }
  return Size{0, 0};
}

// One obstacle: its shape and horizontal position.
struct Pair {
  Shape   shape;
  int16_t x;
};

// The two obstacles in flight at any time -- front (nearer, drives collision) and next (spawned
// ahead of it, becomes front once front recycles).
struct State {
  Pair front{Shape::OneTall, 128};
  Pair next{Shape::OneSmall, 200};
};

// Advances both obstacles by speed; recycles the front one (adopting `next`'s shape/position,
// unchanged this frame -- matching the original) once it scrolls past OFFSCREEN_X, and spawns a
// new far obstacle at the recycled position plus randomGap, with randomShape. randomGap/
// randomShape are injected so this is deterministic/testable -- main.cpp supplies real
// random(80, 125) / random(1, 7) values.
inline void advance(State& s, int16_t speed, int16_t randomGap, Shape randomShape) {
  int16_t frontX = static_cast<int16_t>(s.front.x - speed);
  if (frontX < OFFSCREEN_X) {
    int16_t recycledX = s.next.x;
    s.front = s.next;
    s.front.x = recycledX;
    s.next = Pair{randomShape, static_cast<int16_t>(recycledX + randomGap)};
  } else {
    s.front.x = frontX;
    s.next.x = static_cast<int16_t>(s.next.x - speed);
  }
}

// True if the dino (currently at jumpHeight, 0 = grounded) collides with the front obstacle.
inline bool collides(Shape frontShape, int16_t frontX, uint8_t dinoJumpHeight) {
  Size s = sizeOf(frontShape);
  if (frontX > 15 || frontX + s.width < 5 || dinoJumpHeight > s.height - 3) return false;
  return true;
}

}  // namespace obstacles
