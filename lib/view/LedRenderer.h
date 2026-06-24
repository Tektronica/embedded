#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "HobModel.h"

// View: maps each hob's heat level onto its LED ring. Owns no framebuffer — it writes into the
// CRGB buffer it is given (FastLED.addLeds lives in the entry point so the data pin stays a
// compile-time template argument). Color math is the pure HeatRamp helper.
class LedRenderer {
 public:
  LedRenderer(CRGB* leds, uint8_t hobCount, uint8_t ledsPerRing);
  void render(const HobModel& model);

 private:
  CRGB* leds_;
  uint8_t hobCount_;
  uint8_t ledsPerRing_;
};
