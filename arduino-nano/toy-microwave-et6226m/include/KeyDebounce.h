#pragma once

#include <stdint.h>

#include "ET6226MCodec.h"

// Debounces the ET6226M's raw key readings into a settled, edge-triggered press. Hardware-free:
// given the KeyPosition decoded from the latest readKeyCode() (or {0, 0} for none), reports the
// same fresh-press-only event KeyMatrix.h's Scanner did for the old bit-banged matrix, since
// mechanical switch bounce is a property of the buttons themselves, not eliminated by which chip
// scans them. Only debounces and edge-detects -- unlike KeyMatrix.h, there's no layout lookup
// here, since the ET6226M already resolves which grid/segment was pressed; that lookup moved to
// main.cpp's translateKey(), which maps a KeyPosition to a microwave::Event.
namespace keydebounce {

constexpr uint8_t DEBOUNCE_SCANS = 3;  // consistent scans required before committing a keypress

// Same N-consistent-scan debounce pattern as keymatrix::Scanner, generalized from a raw index to
// an et6226m::KeyPosition.
class Debouncer {
 public:
  // Feed it the KeyPosition decoded from the latest scan (or {0, 0} if nothing is pressed).
  // Returns the freshly-settled KeyPosition on a new press, or {0, 0} if nothing new settled.
  et6226m::KeyPosition scan(et6226m::KeyPosition current) {
    if (current.grid == state_.grid && current.segment == state_.segment) {
      count_ = 0;
      return et6226m::KeyPosition{0, 0};
    }
    if (++count_ < DEBOUNCE_SCANS) return et6226m::KeyPosition{0, 0};
    count_ = 0;
    state_ = current;
    return state_;  // {0, 0} settling in (a release) reports as "no key", same as a fresh press
  }

 private:
  et6226m::KeyPosition state_{0, 0};
  uint8_t count_ = 0;
};

}  // namespace keydebounce
