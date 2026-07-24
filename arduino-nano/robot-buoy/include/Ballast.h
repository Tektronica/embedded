#pragma once

#include <stdint.h>

// Syringe ballast geometry: a stepper-driven syringe draws water in to dive (added mass, less
// buoyant) or expels it to surface (less mass, more buoyant) -- the "buoyancy engine" moving
// fluid across the hull to change displaced volume/density (see README). Hardware-free -- the
// actual AccelStepper drive lives in main.cpp -- so the stroke-length math unit-tests off-device.
namespace ballast {

constexpr uint16_t STEPS_PER_REV = 200;
constexpr uint8_t MICROSTEPS = 32;
constexpr uint8_t FILL_REVOLUTIONS = 5;  // placeholder -- calibrate against real syringe travel

// Total microsteps commanded for one full fill/drain stroke.
constexpr long strokeSteps() {
  return static_cast<long>(MICROSTEPS) * STEPS_PER_REV * FILL_REVOLUTIONS;
}

}  // namespace ballast
