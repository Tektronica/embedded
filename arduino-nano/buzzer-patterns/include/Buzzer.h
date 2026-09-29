#pragma once

#include <stdint.h>

#include "ToneSequence.h"

// Named alert/feedback sounds (KeyPress, Done, Error). The sequencing is lib/ToneSequence; this
// header is only the sound tables. main.cpp maps ToneState to tone()/noTone().
namespace buzzer {

using tonesequence::SILENCE;
using tonesequence::ToneState;

constexpr uint16_t KEYPRESS_HZ      = 2000;
constexpr uint16_t KEYPRESS_MS      = 40;
constexpr uint16_t DONE_BEEP_HZ     = 2500;
constexpr uint16_t DONE_BEEP_MS     = 200;
constexpr uint16_t DONE_BEEP_GAP_MS = 150;
constexpr uint8_t  DONE_BEEP_COUNT  = 4;
constexpr uint16_t ERROR_HZ         = 300;
constexpr uint16_t ERROR_MS         = 400;

enum class Pattern : uint8_t { None, KeyPress, Done, Error };

constexpr tonesequence::Step KEYPRESS_STEPS[] = {{KEYPRESS_HZ, KEYPRESS_MS}};
constexpr tonesequence::Step DONE_STEPS[] = {
    {DONE_BEEP_HZ, DONE_BEEP_MS}, {SILENCE, DONE_BEEP_GAP_MS},
    {DONE_BEEP_HZ, DONE_BEEP_MS}, {SILENCE, DONE_BEEP_GAP_MS},
    {DONE_BEEP_HZ, DONE_BEEP_MS}, {SILENCE, DONE_BEEP_GAP_MS},
    {DONE_BEEP_HZ, DONE_BEEP_MS}, {SILENCE, DONE_BEEP_GAP_MS},
};
static_assert(sizeof(DONE_STEPS) / sizeof(DONE_STEPS[0]) == 2 * DONE_BEEP_COUNT,
              "DONE_STEPS must hold DONE_BEEP_COUNT beep/gap pairs");
constexpr tonesequence::Step ERROR_STEPS[] = {{ERROR_HZ, ERROR_MS}};

inline tonesequence::Sequence sequenceFor(Pattern pattern) {
  switch (pattern) {
    case Pattern::KeyPress: return {KEYPRESS_STEPS, 1, false};
    case Pattern::Done:     return {DONE_STEPS, 2 * DONE_BEEP_COUNT, false};
    case Pattern::Error:    return {ERROR_STEPS, 1, false};
    case Pattern::None:
    default:                return {nullptr, 0, false};
  }
}

inline ToneState toneStateFor(Pattern pattern, uint32_t elapsedMs) {
  return tonesequence::toneStateAt(sequenceFor(pattern), elapsedMs);
}

inline bool isFinished(Pattern pattern, uint32_t elapsedMs) {
  return tonesequence::isFinished(sequenceFor(pattern), elapsedMs);
}

}  // namespace buzzer
