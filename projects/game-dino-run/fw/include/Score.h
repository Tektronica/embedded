#pragma once

#include <stdint.h>

// Score computation and milestone-beep threshold. Hardware-free so it unit-tests off-device.
namespace score {

constexpr uint16_t MILESTONE_INTERVAL = 100;

// Score increases with elapsed play time and current speed -- faster obstacles (harder) also
// score faster.
inline uint16_t compute(uint32_t elapsedMs, uint16_t speed) {
  return static_cast<uint16_t>(elapsedMs * speed / 1000);
}

// A milestone beep plays every MILESTONE_INTERVAL points. Returns the new milestone count once
// score crosses one, or lastMilestone unchanged otherwise -- the caller compares the return
// value against lastMilestone to decide whether to actually play the sound.
inline uint16_t milestoneFor(uint16_t currentScore, uint16_t lastMilestone) {
  uint16_t current = static_cast<uint16_t>(currentScore / MILESTONE_INTERVAL);
  return current > lastMilestone ? current : lastMilestone;
}

}  // namespace score
