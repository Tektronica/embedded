#pragma once

#include <Arduino.h>

#include "Config.h"

// Hardware pin assignments — the single source of truth for wiring (see docs/hardware). Only the
// entry point and hardware-facing modules include this; pure logic never does.
namespace pins {

constexpr uint8_t LED_DATA = 6;  // single data line into ring 1's DIN
constexpr uint8_t HOB_DIMMER[config::HOB_COUNT] = { A0, A1, A2, A3 };

}  // namespace pins
