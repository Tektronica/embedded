#pragma once

#include <stdint.h>

// Named alert/feedback tone patterns (KeyPress, Done, Error): pure frequency/timing logic, no
// Arduino.h, so it unit-tests off-device. main.cpp maps ToneState to tone()/noTone() calls. Same
// shape as toy-microwave's and game-dino-run's Buzzer.h -- standalone here the way `keypad` is
// the standalone counterpart to toy-microwave's matrix-scanning technique.
namespace buzzer {

constexpr uint16_t KEYPRESS_HZ      = 2000;
constexpr uint16_t KEYPRESS_MS      = 40;
constexpr uint16_t DONE_BEEP_HZ     = 2500;
constexpr uint16_t DONE_BEEP_MS     = 200;
constexpr uint16_t DONE_BEEP_GAP_MS = 150;
constexpr uint8_t  DONE_BEEP_COUNT  = 4;
constexpr uint16_t ERROR_HZ         = 300;
constexpr uint16_t ERROR_MS         = 400;

enum class Pattern : uint8_t { None, KeyPress, Done, Error };

// Whether the buzzer should be sounding right now, and at what frequency, `elapsedMs` after
// `pattern` started.
struct ToneState {
  bool     on;
  uint16_t frequencyHz;
};

inline ToneState toneStateFor(Pattern pattern, uint32_t elapsedMs) {
  switch (pattern) {
    case Pattern::KeyPress:
      return ToneState{elapsedMs < KEYPRESS_MS, KEYPRESS_HZ};

    case Pattern::Done: {
      uint16_t cycleMs = DONE_BEEP_MS + DONE_BEEP_GAP_MS;
      uint32_t totalMs = static_cast<uint32_t>(DONE_BEEP_COUNT) * cycleMs;
      if (elapsedMs >= totalMs) return ToneState{false, DONE_BEEP_HZ};
      uint16_t phaseMs = static_cast<uint16_t>(elapsedMs % cycleMs);
      return ToneState{phaseMs < DONE_BEEP_MS, DONE_BEEP_HZ};
    }

    case Pattern::Error:
      return ToneState{elapsedMs < ERROR_MS, ERROR_HZ};

    case Pattern::None:
    default:
      return ToneState{false, 0};
  }
}

// Has this one-shot pattern finished its whole sequence? Note this is NOT simply "!on" -- Done
// oscillates on/off across its 4 beeps and would look "finished" after the first one otherwise.
inline bool isFinished(Pattern pattern, uint32_t elapsedMs) {
  switch (pattern) {
    case Pattern::KeyPress: return elapsedMs >= KEYPRESS_MS;
    case Pattern::Error:    return elapsedMs >= ERROR_MS;
    case Pattern::Done:     return elapsedMs >= static_cast<uint32_t>(DONE_BEEP_COUNT) *
                                                     (DONE_BEEP_MS + DONE_BEEP_GAP_MS);
    case Pattern::None:
    default:                return true;
  }
}

}  // namespace buzzer
