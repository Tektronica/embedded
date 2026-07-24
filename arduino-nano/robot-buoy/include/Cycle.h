#pragma once

#include <stdint.h>

// Vertical-profile dive cycle: Surface -> Descent -> Park -> Ascent -> Surface, repeating
// indefinitely. Phase names and order follow the standard profiling-float cycle (as used by
// real Argo floats): Park is the drift-at-depth phase, distinct from actively descending or
// ascending. Each phase maps to one hardware action in main.cpp; next() is pure so the cycle
// order is unit-tested off-device rather than only verified by watching hardware.
namespace cycle {

enum class Phase : uint8_t { Surface, Descent, Park, Ascent };

inline Phase next(Phase current) {
  switch (current) {
    case Phase::Surface: return Phase::Descent;
    case Phase::Descent: return Phase::Park;
    case Phase::Park:    return Phase::Ascent;
    case Phase::Ascent:  return Phase::Surface;
  }
  return Phase::Surface;
}

}  // namespace cycle
