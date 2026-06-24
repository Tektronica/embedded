#include "CooktopAppliance.h"

CooktopAppliance::CooktopAppliance(CRGB* leds, const uint8_t* dimmerPins, uint8_t hobCount,
                                   uint8_t ledsPerRing)
    : input_(dimmerPins, hobCount), renderer_(leds, hobCount, ledsPerRing) {}

void CooktopAppliance::begin() { input_.begin(); }

void CooktopAppliance::update() { input_.update(model_); }

void CooktopAppliance::render() { renderer_.render(model_); }
