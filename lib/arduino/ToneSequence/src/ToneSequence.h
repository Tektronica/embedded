#pragma once

#include <stdint.h>

// Hardware-free, non-blocking tone sequencing. An app describes each sound as a table of
// {frequency, duration} steps; the loop polls toneStateAt() and maps the result to
// tone()/noTone(), so nothing ever blocks on delay().
namespace tonesequence {

constexpr uint16_t SILENCE = 0;

// Whether the buzzer should be sounding right now, and at what frequency.
struct ToneState {
  bool     on;
  uint16_t frequencyHz;
};

// One step: `frequencyHz` (SILENCE for a gap) held for `durationMs`.
struct Step {
  uint16_t frequencyHz;
  uint16_t durationMs;
};

// A sound: `steps[0..count)` played once, or repeated forever when `loops`.
struct Sequence {
  const Step* steps;
  uint8_t     count;
  bool        loops;
};

inline uint32_t totalMs(const Sequence& sequence) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < sequence.count; ++i) total += sequence.steps[i].durationMs;
  return total;
}

// A one-shot sequence is finished once its last step ends; a looping one never is. An empty
// sequence is always finished.
inline bool isFinished(const Sequence& sequence, uint32_t elapsedMs) {
  uint32_t total = totalMs(sequence);
  if (total == 0) return true;
  return !sequence.loops && elapsedMs >= total;
}

// What to play `elapsedMs` after the sequence started. Silent once a one-shot sequence finishes.
inline ToneState toneStateAt(const Sequence& sequence, uint32_t elapsedMs) {
  uint32_t total = totalMs(sequence);
  if (total == 0) return ToneState{false, SILENCE};
  if (elapsedMs >= total) {
    if (!sequence.loops) return ToneState{false, SILENCE};
    elapsedMs %= total;
  }
  for (uint8_t i = 0; i < sequence.count; ++i) {
    const Step& step = sequence.steps[i];
    if (elapsedMs < step.durationMs) return ToneState{step.frequencyHz != SILENCE, step.frequencyHz};
    elapsedMs -= step.durationMs;
  }
  return ToneState{false, SILENCE};
}

}  // namespace tonesequence
