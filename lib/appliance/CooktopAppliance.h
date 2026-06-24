#pragma once

#include <FastLED.h>

#include "HobModel.h"
#include "IAppliance.h"
#include "InputController.h"
#include "LedRenderer.h"

// The cooktop appliance: hob rings driven by dimmers. Composes the MVC pieces and exposes them
// through the IAppliance plugin contract. Wiring (pins, framebuffer) is injected, not hardcoded.
class CooktopAppliance : public IAppliance {
 public:
  CooktopAppliance(CRGB* leds, const uint8_t* dimmerPins, uint8_t hobCount, uint8_t ledsPerRing);

  void begin() override;
  void update() override;
  void render() override;

 private:
  HobModel model_;
  InputController input_;
  LedRenderer renderer_;
};
