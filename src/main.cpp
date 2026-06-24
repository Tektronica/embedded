#include <Arduino.h>
#include <FastLED.h>

#include "Config.h"
#include "CooktopAppliance.h"
#include "IAppliance.h"
#include "Pins.h"

namespace {
CRGB leds[config::TOTAL_LEDS];
CooktopAppliance cooktop(leds, pins::HOB_DIMMER, config::HOB_COUNT, config::LEDS_PER_RING);
IAppliance* appliance = &cooktop;  // drive whichever appliance via the plugin contract
}  // namespace

void setup() {
  FastLED.addLeds<WS2812B, pins::LED_DATA, GRB>(leds, config::TOTAL_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(config::PSU_VOLTS, config::PSU_MILLIAMPS);
  FastLED.setBrightness(config::MAX_BRIGHTNESS);
  appliance->begin();
}

void loop() {
  appliance->update();
  appliance->render();
  FastLED.delay(config::FRAME_DELAY_MS);  // shows the frame and keeps temporal dithering active
}
