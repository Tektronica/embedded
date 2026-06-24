#pragma once

#include <stdint.h>

// Generic N-channel dimmer → LED-strip controller. Hardware-free (no Arduino/FastLED) so it
// unit-tests off-device. Each channel maps one dimmer input to one LED strip via a level→color curve.
namespace controller {

constexpr uint8_t  CHANNEL_COUNT  = 4;
constexpr uint8_t  LED_STRIP_LENGTH = 35;
constexpr uint16_t LED_TOTAL     = static_cast<uint16_t>(CHANNEL_COUNT) * LED_STRIP_LENGTH;  // 140

// Per-channel level, 0 (off) .. 255 (max).
class Levels {
 public:
  void setLevel(uint8_t ch, uint8_t level) {
    if (ch < CHANNEL_COUNT) levels_[ch] = level;
  }
  uint8_t level(uint8_t ch) const { return ch < CHANNEL_COUNT ? levels_[ch] : 0; }
  uint8_t count() const { return CHANNEL_COUNT; }

 private:
  uint8_t levels_[CHANNEL_COUNT] = {0};
};

// --- Dimmer input logic ---
constexpr uint16_t ADC_MAX = 1023;

// Map a raw 10-bit ADC reading to a level (0..255).
inline uint8_t adcToLevel(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * 255u / ADC_MAX);
}

// One exponential-moving-average step toward `raw`; larger `shift` = smoother/slower.
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  return static_cast<uint16_t>(smoothed + (delta >> shift));
}

// --- Level → color curve ---
struct Hsv {
  uint8_t h;
  uint8_t s;
  uint8_t v;
};

// Default curve: off -> dim deep red -> bright orange (full saturation, hue capped at orange).
// Hue values are interpreted by FastLED's default *rainbow* map (hsv2rgb_rainbow): 0 = red,
// 32 = orange, 64 = yellow. Capping at 32 means it never reaches yellow. (Literal numbers rather
// than FastLED's HUE_* names keep this header hardware-free and unit-testable.) Swap this function
// to change the rendering; it carries no application meaning.
inline Hsv levelColor(uint8_t level) {
  if (level == 0) return Hsv{0, 0, 0};
  uint16_t t = static_cast<uint16_t>(level) - 1;                    // 0..254
  uint8_t h = static_cast<uint8_t>(t * 32u / 254u);                 // rainbow hue 0=red -> 32=HUE_ORANGE
  uint8_t v = static_cast<uint8_t>(48u + t * (255u - 48u) / 254u);  // 48 -> 255 brightness
  return Hsv{h, 255, v};                                            // full saturation
}

}  // namespace controller
