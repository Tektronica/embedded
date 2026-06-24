#include <Arduino.h>
#include <FastLED.h>

#include "Cooktop.h"

namespace
{

  // Board wiring + power — the only hardware-specific bits (see docs/hardware).
  constexpr uint8_t LED_DATA_PIN = 6;
  constexpr uint8_t DIMMER_PINS[cooktop::HOB_COUNT] = {A0, A1, A2, A3};
  constexpr uint8_t PSU_VOLTS = 5;
  constexpr uint16_t PSU_MILLIAMPS = 5500;
  constexpr uint8_t MAX_BRIGHTNESS = 255; // full brightness; PSU_MILLIAMPS cap below still protects real hardware
  constexpr uint8_t FRAME_DELAY_MS = 16; // ~60 fps
  constexpr uint8_t EMA_SHIFT = 3;       // dimmer smoothing strength

  CRGB leds[cooktop::TOTAL_LEDS];
  cooktop::HobModel model;
  uint16_t smoothed[cooktop::HOB_COUNT];

} // namespace

void setup()
{
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, cooktop::TOTAL_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(PSU_VOLTS, PSU_MILLIAMPS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  for (uint8_t i = 0; i < cooktop::HOB_COUNT; ++i)
  {
    pinMode(DIMMER_PINS[i], INPUT);
    smoothed[i] = 0;
  }
}

void loop()
{
  // read inputs -> update model
  for (uint8_t i = 0; i < cooktop::HOB_COUNT; ++i)
  {
    smoothed[i] = cooktop::emaStep(smoothed[i], analogRead(DIMMER_PINS[i]), EMA_SHIFT);
    model.setLevel(i, cooktop::adcToLevel(smoothed[i]));
  }

  // render model -> rings (ring N owns LED indices [N*35, N*35+34])
  for (uint8_t hob = 0; hob < cooktop::HOB_COUNT; ++hob)
  {
    cooktop::Hsv c = cooktop::heatColor(model.level(hob));
    CRGB color = (c.v == 0) ? CRGB(CRGB::Black) : CRGB(CHSV(c.h, c.s, c.v));
    uint16_t start = static_cast<uint16_t>(hob) * cooktop::LEDS_PER_RING;
    for (uint8_t i = 0; i < cooktop::LEDS_PER_RING; ++i)
      leds[start + i] = color;
  }

  FastLED.show();
  FastLED.delay(FRAME_DELAY_MS); // shows the frame and keeps temporal dithering active
}
