#pragma once

#include <stdint.h>

// Pure input logic: ADC scaling and smoothing, hardware-free for off-device testing.
namespace inputmap {

constexpr uint16_t ADC_MAX = 1023;

// Map a raw 10-bit ADC reading to a heat level (0..255).
inline uint8_t adcToLevel(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * 255u / ADC_MAX);
}

// One exponential-moving-average step toward `raw`; larger `shift` = smoother/slower.
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  return static_cast<uint16_t>(smoothed + (delta >> shift));
}

}  // namespace inputmap
