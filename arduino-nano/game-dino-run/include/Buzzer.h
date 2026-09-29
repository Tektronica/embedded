#pragma once

#include <stdint.h>

#include "ToneSequence.h"

// Game sound effects (jump, milestone, hit). The sequencing is lib/ToneSequence, polled every loop
// rather than blocking on delay(); this header is only the sound tables. main.cpp maps ToneState
// to tone()/noTone().
namespace buzzer {

using tonesequence::SILENCE;
using tonesequence::ToneState;

constexpr uint16_t JUMP_HZ = 700;
constexpr uint16_t JUMP_MS = 100;

constexpr uint16_t MILESTONE_TONE1_HZ = 1000;
constexpr uint16_t MILESTONE_TONE2_HZ = 1250;
constexpr uint16_t MILESTONE_TONE_MS  = 100;
constexpr uint16_t MILESTONE_GAP_MS   = 150;  // start-to-start gap between the two tones

constexpr uint16_t HIT_HZ      = 125;
constexpr uint16_t HIT_TONE_MS = 100;
constexpr uint16_t HIT_GAP_MS  = 150;  // start-to-start gap between the two tones

enum class Pattern : uint8_t { None, Jump, Milestone, Hit };

constexpr tonesequence::Step JUMP_STEPS[] = {{JUMP_HZ, JUMP_MS}};
constexpr tonesequence::Step MILESTONE_STEPS[] = {
    {MILESTONE_TONE1_HZ, MILESTONE_TONE_MS},
    {SILENCE, MILESTONE_GAP_MS - MILESTONE_TONE_MS},
    {MILESTONE_TONE2_HZ, MILESTONE_TONE_MS},
};
constexpr tonesequence::Step HIT_STEPS[] = {
    {HIT_HZ, HIT_TONE_MS},
    {SILENCE, HIT_GAP_MS - HIT_TONE_MS},
    {HIT_HZ, HIT_TONE_MS},
};

inline tonesequence::Sequence sequenceFor(Pattern pattern) {
  switch (pattern) {
    case Pattern::Jump:      return {JUMP_STEPS, 1, false};
    case Pattern::Milestone: return {MILESTONE_STEPS, 3, false};
    case Pattern::Hit:       return {HIT_STEPS, 3, false};
    case Pattern::None:
    default:                 return {nullptr, 0, false};
  }
}

inline ToneState toneStateFor(Pattern pattern, uint32_t elapsedMs) {
  return tonesequence::toneStateAt(sequenceFor(pattern), elapsedMs);
}

inline bool isFinished(Pattern pattern, uint32_t elapsedMs) {
  return tonesequence::isFinished(sequenceFor(pattern), elapsedMs);
}

}  // namespace buzzer
