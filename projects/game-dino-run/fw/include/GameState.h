#pragma once

#include <stdint.h>

// Top-level game state: Start -> Playing -> End -> Start. Hardware-free so it unit-tests
// off-device. Collision-driven End is a separate trigger from a button press (main.cpp sets it
// directly when obstacles::collides() is true) -- next() models only what a button press does.
namespace gamestate {

// Which screen/mode the game is in.
enum class State : uint8_t { Start, Playing, End };

// A button press's effect on game state depends only on the current state: Start begins
// Playing; End returns to Start (a second press is needed to play again, matching the original
// design). A press while Playing has no state-transition effect here -- it triggers a jump
// instead, handled separately (see Dino.h) since a jump doesn't change game state.
inline State next(State current) {
  switch (current) {
    case State::Start:   return State::Playing;
    case State::Playing: return State::Playing;
    case State::End:     return State::Start;
  }
  return State::Start;
}

}  // namespace gamestate
