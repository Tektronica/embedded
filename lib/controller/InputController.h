#pragma once

#include <Arduino.h>

#include "Config.h"
#include "HobModel.h"

// Controller: reads the per-hob dimmer pots, smooths them (see InputMap), and writes heat levels
// into the model. Pins are injected so the controller owns no wiring knowledge.
class InputController {
 public:
  InputController(const uint8_t* dimmerPins, uint8_t count);
  void begin();
  void update(HobModel& model);

 private:
  const uint8_t* pins_;
  uint8_t count_;
  uint16_t smoothed_[config::HOB_COUNT];  // EMA of the raw ADC reading (0..1023)
};
