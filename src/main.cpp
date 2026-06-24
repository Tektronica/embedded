#include <Arduino.h>
#include <FastLED.h>

#include "LEDStripDimmer.h"

namespace {

// Pin assignments (see docs/hardware). The two switch pins are optional: INPUT_PULLUP means an
// unwired switch reads HIGH, never registers a press, and stays on the default mode/palette.
constexpr uint8_t  PIN_DATA_LED     = 6;
constexpr uint8_t  PIN_DIMMER[controller::CHANNEL_COUNT] = {A0, A1, A2, A3};
constexpr uint8_t  PIN_SWITCH_MODE  = 8;  // optional — cycles Mode (default Solid)
constexpr uint8_t  PIN_SWITCH_COLOR = 9;  // optional — cycles Palette (default HeatRedOrange)

// Power, brightness, and frame timing.
constexpr uint8_t  PSU_VOLTS        = 5;
constexpr uint16_t PSU_MILLIAMPS    = 5500;
constexpr uint8_t  LED_BRIGHTNESS_MAX = 255;  // full; PSU_MILLIAMPS cap below still protects hardware
constexpr uint8_t  FRAME_DELAY_MS   = 16;     // ~60 fps
constexpr uint8_t  EMA_SHIFT        = 3;      // dimmer smoothing strength

CRGB leds[controller::LED_TOTAL];
controller::Levels levels;
uint16_t smoothed[controller::CHANNEL_COUNT];

controller::Mode    mode    = controller::Mode::Solid;            // default when the switch is unwired
controller::Palette palette = controller::Palette::HeatRedOrange; // default when the switch is unwired
controller::Button modeButton;
controller::Button colorButton;
uint16_t frame = 0;

}  // namespace

void setup() {
  FastLED.addLeds<WS2812B, PIN_DATA_LED, GRB>(leds, controller::LED_TOTAL);
  FastLED.setMaxPowerInVoltsAndMilliamps(PSU_VOLTS, PSU_MILLIAMPS);
  FastLED.setBrightness(LED_BRIGHTNESS_MAX);
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    pinMode(PIN_DIMMER[ch], INPUT);
    smoothed[ch] = 0;
  }
  pinMode(PIN_SWITCH_MODE, INPUT_PULLUP);    // unwired reads HIGH -> never a press -> default
  pinMode(PIN_SWITCH_COLOR, INPUT_PULLUP);
}

void loop() {
  // read dimmer inputs -> levels
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    smoothed[ch] = controller::emaStep(smoothed[ch], analogRead(PIN_DIMMER[ch]), EMA_SHIFT);
    levels.setLevel(ch, controller::adcToLevel(smoothed[ch]));
  }

  // read switches (INPUT_PULLUP: LOW = pressed); a fresh press cycles mode / palette
  if (modeButton.pressed(digitalRead(PIN_SWITCH_MODE) == LOW)) mode = controller::nextMode(mode);
  if (colorButton.pressed(digitalRead(PIN_SWITCH_COLOR) == LOW)) palette = controller::nextPalette(palette);

  // render levels -> LED strip outputs (channel N owns LED indices [N*LED_STRIP_LENGTH, ...])
  for (uint8_t ch = 0; ch < controller::CHANNEL_COUNT; ++ch) {
    uint8_t level = levels.level(ch);
    uint16_t start = static_cast<uint16_t>(ch) * controller::LED_STRIP_LENGTH;
    for (uint8_t i = 0; i < controller::LED_STRIP_LENGTH; ++i) {
      controller::Hsv c = controller::pixelColor(palette, mode, level, i, controller::LED_STRIP_LENGTH, frame);
      leds[start + i] = (c.v == 0) ? CRGB(CRGB::Black) : CRGB(CHSV(c.h, c.s, c.v));
    }
  }

  FastLED.show();
  FastLED.delay(FRAME_DELAY_MS);  // shows the frame and keeps temporal dithering active
  ++frame;
}
