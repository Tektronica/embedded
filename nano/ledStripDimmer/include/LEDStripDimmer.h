#pragma once

#include <stdint.h>

// Generic N-channel dimmer → LED-strip controller. Hardware-free (no Arduino/FastLED) so it
// unit-tests off-device. Each channel maps one dimmer input to one LED strip; an optional color
// switch cycles the palette and an optional mode switch cycles the animation. Both default to
// index 0 when unwired (see main.cpp's INPUT_PULLUP).
namespace controller {

constexpr uint8_t  CHANNEL_COUNT    = 4;
constexpr uint8_t  LED_STRIP_LENGTH = 35;
constexpr uint16_t LED_TOTAL        = static_cast<uint16_t>(CHANNEL_COUNT) * LED_STRIP_LENGTH;  // 140

constexpr uint16_t ADC_MAX = 1023;

// Hue anchors, FastLED rainbow map (hsv2rgb_rainbow): 0=red, 16=orange-red, 32=orange, 64=yellow,
// 96=green, 160=blue. Literal numbers (not FastLED's HUE_*) keep this header hardware-free.
constexpr uint8_t HUE_RED        = 0;
constexpr uint8_t HUE_ORANGE_RED = 16;
constexpr uint8_t HUE_GREEN      = 96;
constexpr uint8_t HUE_BLUE       = 160;

// Color switch cycles this; [0] = HeatRedOrange is the unwired default.
enum class Palette : uint8_t { HeatRedOrange, Green, Blue, White, Rainbow, Count };
// Mode switch cycles this; [0] = Solid is the unwired default.
enum class Mode : uint8_t { Solid, Blink, Strobe, Chase, Count };

struct Hsv {
  uint8_t h;
  uint8_t s;
  uint8_t v;
};

// --- State: per-channel level (0..255) ---
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

// Map a raw 10-bit ADC reading to a level (0..255).
inline uint8_t adcToLevel(uint16_t raw) {
  if (raw > ADC_MAX) raw = ADC_MAX;
  return static_cast<uint8_t>(static_cast<uint32_t>(raw) * 255u / ADC_MAX);
}

// One EMA step toward `raw`; larger `shift` = smoother/slower. The `step != 0` guard removes the
// integer dead-band so it converges exactly (dimmer at max really reaches full brightness).
inline uint16_t emaStep(uint16_t smoothed, uint16_t raw, uint8_t shift) {
  int16_t delta = static_cast<int16_t>(raw - smoothed);
  int16_t step = static_cast<int16_t>(delta >> shift);
  if (step == 0 && delta != 0) step = (delta > 0) ? 1 : -1;
  return static_cast<uint16_t>(smoothed + step);
}

// --- Switch cycling (wrap past Count back to 0) ---
inline Palette nextPalette(Palette p) {
  uint8_t n = static_cast<uint8_t>(p) + 1;
  if (n >= static_cast<uint8_t>(Palette::Count)) n = 0;
  return static_cast<Palette>(n);
}
inline Mode nextMode(Mode m) {
  uint8_t n = static_cast<uint8_t>(m) + 1;
  if (n >= static_cast<uint8_t>(Mode::Count)) n = 0;
  return static_cast<Mode>(n);
}

// --- Color (palette) ---

// Brightness (HSV value) from the dimmer level: off at 0, else a 48..255 ramp.
inline uint8_t levelBrightness(uint8_t level) {
  if (level == 0) return 0;
  uint16_t t = static_cast<uint16_t>(level) - 1;                  // 0..254
  return static_cast<uint8_t>(48u + t * (255u - 48u) / 254u);     // 48 -> 255
}

inline uint8_t paletteSaturation(Palette pal) {
  return (pal == Palette::White) ? 0 : 255;  // White = fully desaturated
}

// Hue for a pixel. HeatRedOrange ramps red→orange-red with level; Rainbow spans one full cycle
// across the LED strip's pixels; the rest are a fixed hue.
inline uint8_t paletteHue(Palette pal, uint8_t level, uint8_t pixelIndex, uint8_t stripLength) {
  switch (pal) {
    case Palette::HeatRedOrange: {
      uint16_t t = (level == 0) ? 0 : static_cast<uint16_t>(level) - 1;
      return static_cast<uint8_t>(t * HUE_ORANGE_RED / 254u);     // 0 (red) -> 16 (orange-red)
    }
    case Palette::Green:   return HUE_GREEN;
    case Palette::Blue:    return HUE_BLUE;
    case Palette::White:   return HUE_RED;  // hue irrelevant at saturation 0
    case Palette::Rainbow:
      return (stripLength == 0) ? 0
             : static_cast<uint8_t>(static_cast<uint16_t>(pixelIndex) * 256u / stripLength);
    default: return HUE_RED;
  }
}

// --- Mode (animation) — frame-based; the loop advances `frame` ~60×/s ---
namespace anim {
constexpr uint16_t BLINK_PERIOD   = 60;  // ~1 Hz: on for the first half
constexpr uint16_t STROBE_PERIOD  = 6;   // ~10 Hz brief flashes
constexpr uint16_t STROBE_ON      = 1;   // frames lit per strobe period
constexpr uint16_t CHASE_STEP     = 3;   // frames per 1-pixel advance
constexpr uint8_t  CHASE_WIDTH    = 6;   // lit segment length (pixels)
}  // namespace anim

// Is this pixel lit this frame for the given mode? (Spatial/temporal gate, independent of color.)
inline bool modeLit(Mode mode, uint8_t pixelIndex, uint8_t stripLength, uint16_t frame) {
  switch (mode) {
    case Mode::Solid:  return true;
    case Mode::Blink:  return (frame % anim::BLINK_PERIOD) < (anim::BLINK_PERIOD / 2);
    case Mode::Strobe: return (frame % anim::STROBE_PERIOD) < anim::STROBE_ON;
    case Mode::Chase: {
      if (stripLength == 0) return false;
      uint16_t pos = (frame / anim::CHASE_STEP) % stripLength;
      uint16_t rel = (static_cast<uint16_t>(pixelIndex) + stripLength - pos) % stripLength;
      return rel < anim::CHASE_WIDTH;
    }
    default: return true;
  }
}

// The one pure render primitive: color of one pixel given palette, mode, dimmer level, position,
// and the animation frame. Returns {0,0,0} (off) when the level is 0 or the mode gates it off.
inline Hsv pixelColor(Palette pal, Mode mode, uint8_t level, uint8_t pixelIndex,
                      uint8_t stripLength, uint16_t frame) {
  uint8_t v = levelBrightness(level);
  if (v == 0 || !modeLit(mode, pixelIndex, stripLength, frame)) return Hsv{0, 0, 0};
  return Hsv{paletteHue(pal, level, pixelIndex, stripLength), paletteSaturation(pal), v};
}

// --- Debounced push switch (pure). Feed the raw "pressed" state once per frame; returns true
// once on each fresh press (rising edge) after DEBOUNCE consistent samples. ---
class Button {
 public:
  bool pressed(bool raw) {
    if (raw == state_) {
      count_ = 0;
      return false;
    }
    if (++count_ < DEBOUNCE) return false;  // not yet stable
    count_ = 0;
    state_ = raw;       // commit the new debounced state
    return state_;      // a fresh press only (true when committing pressed)
  }

 private:
  static constexpr uint8_t DEBOUNCE = 3;
  bool state_ = false;     // committed state: false = released
  uint8_t count_ = 0;      // consecutive samples differing from state_
};

}  // namespace controller
