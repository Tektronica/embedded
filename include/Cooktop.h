#pragma once

#include <stdint.h>

// All hardware-free cooktop logic — no Arduino.h / FastLED here, so it unit-tests off-device.
// The Arduino glue (pins, FastLED, the loop) lives in src/main.cpp.
namespace cooktop {

constexpr uint8_t  HOB_COUNT     = 4;
constexpr uint8_t  LEDS_PER_RING = 35;
constexpr uint16_t TOTAL_LEDS    = static_cast<uint16_t>(HOB_COUNT) * LEDS_PER_RING;  // 140

// State: heat level per hob, 0 (off) .. 255 (max).
class HobModel {
 public:
  void setLevel(uint8_t hob, uint8_t level) {
    if (hob < HOB_COUNT) levels_[hob] = level;
  }
  uint8_t level(uint8_t hob) const { return hob < HOB_COUNT ? levels_[hob] : 0; }
  uint8_t hobCount() const { return HOB_COUNT; }

 private:
  uint8_t levels_[HOB_COUNT] = {0};
};

// --- Input logic ---
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

// --- Heat color ramp ---
struct Hsv {
  uint8_t h;
  uint8_t s;
  uint8_t v;
};

// Off -> dim deep red -> orange -> bright yellow, desaturating toward white when very hot.
inline Hsv heatColor(uint8_t level) {
  if (level == 0) return Hsv{0, 0, 0};
  uint16_t t = static_cast<uint16_t>(level) - 1;                    // 0..254
  uint8_t h = static_cast<uint8_t>(t * 64u / 254u);                 // hue 0 (red) -> 64 (yellow)
  uint8_t v = static_cast<uint8_t>(48u + t * (255u - 48u) / 254u);  // 48 -> 255
  uint8_t s = 255;
  if (level >= 220) {
    s = static_cast<uint8_t>(255u - static_cast<uint16_t>(level - 220) * (255u - 180u) / (255u - 220u));
  }
  return Hsv{h, s, v};
}

}  // namespace cooktop
