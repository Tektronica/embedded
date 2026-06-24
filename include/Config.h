#pragma once

#include <stdint.h>

// Pure configuration: counts and limits with no hardware dependency, so any model/logic that
// includes this stays unit-testable off-device. Pin assignments live in Pins.h.
namespace config {

constexpr uint8_t  HOB_COUNT     = 4;
constexpr uint8_t  LEDS_PER_RING = 35;
constexpr uint16_t TOTAL_LEDS    = static_cast<uint16_t>(HOB_COUNT) * LEDS_PER_RING;  // 140

constexpr uint8_t  PSU_VOLTS      = 5;
constexpr uint16_t PSU_MILLIAMPS  = 5500;
constexpr uint8_t  MAX_BRIGHTNESS = 200;

constexpr uint8_t  FRAME_DELAY_MS = 16;  // ~60 fps

}  // namespace config
