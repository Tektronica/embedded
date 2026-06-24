#include "InputController.h"

#include "InputMap.h"

namespace {
constexpr uint8_t EMA_SHIFT = 3;  // smoothing strength
}  // namespace

InputController::InputController(const uint8_t* dimmerPins, uint8_t count)
    : pins_(dimmerPins), count_(count) {
  for (uint8_t i = 0; i < config::HOB_COUNT; ++i) smoothed_[i] = 0;
}

void InputController::begin() {
  for (uint8_t i = 0; i < count_; ++i) pinMode(pins_[i], INPUT);
}

void InputController::update(HobModel& model) {
  for (uint8_t i = 0; i < count_; ++i) {
    uint16_t raw = analogRead(pins_[i]);
    smoothed_[i] = inputmap::emaStep(smoothed_[i], raw, EMA_SHIFT);
    model.setLevel(i, inputmap::adcToLevel(smoothed_[i]));
  }
}
