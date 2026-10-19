#pragma once

#include <stdint.h>

// Dino run-cycle animation and jump physics. Hardware-free -- pure integer state transitions --
// so it unit-tests off-device.
//
// legFrame and jumpHeight are two independent pieces of state: which came from one overloaded
// variable in the original sketch, disambiguated only by numeric range (0..2 meant "run-cycle
// frame", 8..32 meant "jump height"). Splitting them fixes a real (if minor) side effect of that
// overload: a jump's height used to start from whatever run-cycle frame (0, 1, or 2) happened to
// be active the instant you jumped, instead of always starting cleanly at 0.
namespace dino {

// Where the dino is in a jump, if any.
enum class JumpPhase : uint8_t { Grounded, Ascending, Descending };

constexpr uint8_t JUMP_STEP = 8;
constexpr uint8_t JUMP_PEAK = 32;
constexpr uint8_t JUMP_LAND_THRESHOLD = 8;
constexpr uint8_t LEG_FRAME_COUNT = 3;

// The dino's current animation/physics state -- see the file comment above for why legFrame and
// jumpHeight are separate fields.
struct State {
  JumpPhase phase = JumpPhase::Grounded;
  uint8_t   legFrame = 0;    // 0..2, cycles while grounded; frozen (unused) while airborne
  uint8_t   jumpHeight = 0;  // 0 while grounded, ramps up then down while airborne
};

// Starts a jump if grounded; returns true if a jump was actually started. Returns false (and
// does nothing) if already airborne, matching the original's "ignore a press while already
// jumping" behavior -- the caller uses the return value to decide whether to play the jump sound.
inline bool startJump(State& s) {
  if (s.phase != JumpPhase::Grounded) return false;
  s.phase = JumpPhase::Ascending;
  return true;
}

// Advances one frame: cycles the run-cycle animation while grounded, or steps the jump arc while
// airborne (ascend to JUMP_PEAK, then descend back to JUMP_LAND_THRESHOLD and land).
inline void advance(State& s) {
  if (s.phase == JumpPhase::Grounded) {
    s.legFrame = static_cast<uint8_t>((s.legFrame + 1) % LEG_FRAME_COUNT);
    return;
  }
  if (s.phase == JumpPhase::Ascending) {
    s.jumpHeight += JUMP_STEP;
    if (s.jumpHeight > JUMP_PEAK) s.phase = JumpPhase::Descending;
    return;
  }
  // Descending
  s.jumpHeight -= JUMP_STEP;
  if (s.jumpHeight < JUMP_LAND_THRESHOLD) {
    s.phase = JumpPhase::Grounded;
    s.jumpHeight = 0;
  }
}

}  // namespace dino
