#pragma once

#include <stdint.h>

// Potentiometer -> stepper speed. Hardware-free (no Arduino/AVR headers) so it unit-tests
// off-device. One pot sets rotation speed (0..MAX_SPEED_STEPS_PER_SEC), rendered by one of two
// strategies in main.cpp -- the AccelStepper library's constant-speed mode, or a from-scratch
// step-pulse generator -- see README's "Stepper strategy" section for why both exist. Knows
// nothing about the run/stop or direction buttons (see Button.h) -- main.cpp reads those and
// passes plain speed/clockwise values in.
namespace stepper {

constexpr uint16_t ADC_MAX = 1023;
constexpr uint16_t MAX_SPEED_STEPS_PER_SEC = 800;

// Map a raw 10-bit ADC reading to a target speed (0..MAX_SPEED_STEPS_PER_SEC).
inline uint16_t potToSpeed(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint16_t>(static_cast<uint32_t>(raw) * MAX_SPEED_STEPS_PER_SEC / ADC_MAX);
}

// One EMA step toward `raw`; larger `shift` = smoother/slower. The `step != 0` guard removes the
// integer dead-band so it converges exactly (pot at max really reaches max speed).
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  int16_t step = static_cast<int16_t>(delta >> shift);
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return static_cast<uint16_t>(smoothed + step);
}

// Microseconds between step pulses for a target speed (direct-pulse path only). Undefined for
// speed 0 -- the caller should skip stepping entirely rather than call this with 0.
inline uint32_t stepIntervalMicros(uint16_t stepsPerSec) {
  return 1000000UL / stepsPerSec;
}

}  // namespace stepper
