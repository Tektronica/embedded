#pragma once

#include <stdint.h>

// Generic N-channel potentiometer → PWM duty-cycle controller. Hardware-free (no Arduino headers)
// so it unit-tests off-device. Each channel is one pot input -> one PWM output.
namespace pwm {

constexpr uint8_t  CHANNEL_COUNT = 4;  // this board has 4 channels; the one constant to change
constexpr uint16_t ADC_MAX       = 1023;
constexpr uint8_t  DUTY_MAX      = 100;
constexpr uint8_t  PWM_MAX       = 255;  // analogWrite()'s 8-bit range

// Map a raw 10-bit ADC reading to a duty percentage (0..100).
inline uint8_t dutyFromAdc(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * DUTY_MAX / ADC_MAX);
}

// One EMA step toward `raw`; larger `shift` = smoother/slower. The `step != 0` guard removes the
// integer dead-band so it converges exactly (pot at max really reaches 100% duty).
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  int16_t step = static_cast<int16_t>(delta >> shift);
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return static_cast<uint16_t>(smoothed + step);
}

// Duty percentage (0..100) → analogWrite() value (0..255).
inline uint8_t dutyToPwm8(uint8_t dutyPercent) {
  if (dutyPercent > DUTY_MAX) dutyPercent = DUTY_MAX;
  return static_cast<uint8_t>(static_cast<uint16_t>(dutyPercent) * PWM_MAX / DUTY_MAX);
}

}  // namespace pwm
