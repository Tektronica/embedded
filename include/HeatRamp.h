#pragma once

#include <stdint.h>

// Pure presentation logic: maps a hob heat level (0..255) to an HSV color. Header-only and
// hardware-free so it can be unit-tested off-device; the view turns this into a FastLED CHSV.
namespace heatramp {

struct Hsv {
  uint8_t h;
  uint8_t s;
  uint8_t v;
};

// Off -> dim deep red -> orange -> bright yellow, desaturating toward white when very hot.
inline Hsv colorFor(uint8_t level) {
  if (level == 0) return Hsv{0, 0, 0};
  uint16_t t = static_cast<uint16_t>(level) - 1;                      // 0..254
  uint8_t h = static_cast<uint8_t>(t * 64u / 254u);                   // hue 0 (red) -> 64 (yellow)
  uint8_t v = static_cast<uint8_t>(48u + t * (255u - 48u) / 254u);    // 48 -> 255
  uint8_t s = 255;
  if (level >= 220) {  // desaturate the hottest range toward white
    s = static_cast<uint8_t>(255u - static_cast<uint16_t>(level - 220) * (255u - 180u) / (255u - 220u));
  }
  return Hsv{h, s, v};
}

}  // namespace heatramp
