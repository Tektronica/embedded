#include <Arduino.h>
#include <FastLED.h>

#include "LEDStripDimmer.h"

namespace {

// Board wiring + power — the only hardware-specific bits (see docs/hardware).
constexpr uint8_t  PIN_DATA_LED  = 6;
constexpr uint8_t  PIN_DIMMER[controller::CHANNEL_COUNT] = {A0, A1, A2, A3};
constexpr uint8_t  PSU_VOLTS      = 5;
constexpr uint16_t PSU_MILLIAMPS  = 5500;
constexpr uint8_t  LED_BRIGHTNESS_MAX = 255;  // full; PSU_MILLIAMPS cap below still protects real hardware
constexpr uint8_t  FRAME_DELAY_MS = 16;   // ~60 fps
constexpr uint8_t  EMA_SHIFT      = 3;    // dimmer smoothing strength

CRGB leds[controller::LED_TOTAL];
controller::Levels levels;
uint16_t smoothed[controller::CHANNEL_COUNT];

}  // namespace

void setup() {
  FastLED.addLeds<WS2812B, PIN_DATA_LED, GRB>(leds, controller::LED_TOTAL);
  FastLED.setMaxPowerInVoltsAndMilliamps(PSU_VOLTS, PSU_MILLIAMPS);
  FastLED.setBrightness(LED_BRIGHTNESS_MAX);
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    pinMode(PIN_DIMMER[ch], INPUT);
    smoothed[ch] = 0;
  }
}

void loop() {
  // read dimmer inputs -> levels
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    smoothed[ch] = controller::emaStep(smoothed[ch], analogRead(PIN_DIMMER[ch]), EMA_SHIFT);
    levels.setLevel(ch, controller::adcToLevel(smoothed[ch]));
  }

  // render levels -> LED strip outputs (channel N owns LED indices [N*LED_STRIP_LENGTH, ...])
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    controller::Hsv c = controller::levelColor(levels.level(ch));
    CRGB color = CRGB::Black;
    if (c.v != 0) color = CHSV(c.h, c.s, c.v);
    uint16_t start = static_cast<uint16_t>(ch) * controller::LED_STRIP_LENGTH;
    for (uint8_t i = 0; i < controller::LED_STRIP_LENGTH; ++i) leds[start + i] = color;
  }

  FastLED.show();
  FastLED.delay(FRAME_DELAY_MS);  // shows the frame and keeps temporal dithering active
}
