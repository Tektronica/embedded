#pragma once

#include <stdint.h>

// Passive buzzer: tone/frequency selection and beep-pattern sequencing (jump, milestone, hit).
// Hardware-free so it unit-tests off-device; main.cpp maps ToneState to actual tone()/noTone()
// calls, polled every loop rather than blocking on delay() -- the original played its two-tone
// patterns with a blocking delay() between tones, freezing animation/input for 150ms each time
// a milestone or hit fired. Same pattern as arduino-nano/toy-microwave's Buzzer.h.
namespace buzzer {

constexpr uint16_t JUMP_HZ = 700;
constexpr uint16_t JUMP_MS = 100;

constexpr uint16_t MILESTONE_TONE1_HZ = 1000;
constexpr uint16_t MILESTONE_TONE2_HZ = 1250;
constexpr uint16_t MILESTONE_TONE_MS  = 100;
constexpr uint16_t MILESTONE_GAP_MS   = 150;  // start-to-start gap between the two tones

constexpr uint16_t HIT_HZ      = 125;
constexpr uint16_t HIT_TONE_MS = 100;
constexpr uint16_t HIT_GAP_MS  = 150;  // start-to-start gap between the two tones

// A sound effect the buzzer can play; None is silence.
enum class Pattern : uint8_t { None, Jump, Milestone, Hit };

// Whether the buzzer should be sounding right now, and at what frequency, `elapsedMs` after
// `pattern` started.
struct ToneState {
  bool     on;
  uint16_t frequencyHz;
};

// What the buzzer should be doing right now for a given pattern and elapsed time -- the caller
// polls this every loop and drives tone()/noTone() from the result, rather than blocking.
inline ToneState toneStateFor(Pattern pattern, uint32_t elapsedMs) {
  switch (pattern) {
    case Pattern::Jump:
      return ToneState{elapsedMs < JUMP_MS, JUMP_HZ};

    case Pattern::Milestone:
      if (elapsedMs < MILESTONE_TONE_MS) return ToneState{true, MILESTONE_TONE1_HZ};
      if (elapsedMs < MILESTONE_GAP_MS) return ToneState{false, 0};
      if (elapsedMs < MILESTONE_GAP_MS + MILESTONE_TONE_MS) return ToneState{true, MILESTONE_TONE2_HZ};
      return ToneState{false, 0};

    case Pattern::Hit:
      if (elapsedMs < HIT_TONE_MS) return ToneState{true, HIT_HZ};
      if (elapsedMs < HIT_GAP_MS) return ToneState{false, 0};
      if (elapsedMs < HIT_GAP_MS + HIT_TONE_MS) return ToneState{true, HIT_HZ};
      return ToneState{false, 0};

    case Pattern::None:
    default:
      return ToneState{false, 0};
  }
}

// Has this one-shot pattern finished its whole sequence? Note this is NOT simply "!on" -- both
// Milestone and Hit go silent during their mid-sequence gap and would look "finished" after the
// first tone otherwise.
inline bool isFinished(Pattern pattern, uint32_t elapsedMs) {
  switch (pattern) {
    case Pattern::Jump:      return elapsedMs >= JUMP_MS;
    case Pattern::Milestone: return elapsedMs >= static_cast<uint32_t>(MILESTONE_GAP_MS) + MILESTONE_TONE_MS;
    case Pattern::Hit:       return elapsedMs >= static_cast<uint32_t>(HIT_GAP_MS) + HIT_TONE_MS;
    case Pattern::None:
    default:                 return true;
  }
}

}  // namespace buzzer
