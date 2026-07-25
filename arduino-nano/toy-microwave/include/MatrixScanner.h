#pragma once

#include <stdint.h>
#include <Arduino.h>

#include "KeyMatrix.h"

// Generic row/column matrix GPIO scanner: drives each row low in turn and reads back which
// column (if any) is pulled low, returning a raw index (row*COLS+col), or keymatrix::NO_KEY if
// none are active. Knows nothing about debouncing or key characters -- see KeyMatrix.h for that
// -- purely the physical scan technique, reusing its ROWS/COLS/NO_KEY constants so the two stay
// in sync. Hardware-coupled (digitalWrite/digitalRead), so this doesn't unit-test off-device.
// Standalone version (identical technique, own tests) lives at arduino-nano/keypad.
namespace matrixscanner {

class Scanner {
 public:
  Scanner(const uint8_t* rowPins, const uint8_t* colPins) : rowPins_(rowPins), colPins_(colPins) {}

  void begin() {
    for (uint8_t r = 0; r < keymatrix::ROWS; ++r) {
      pinMode(rowPins_[r], OUTPUT);
      digitalWrite(rowPins_[r], HIGH);
    }
    for (uint8_t c = 0; c < keymatrix::COLS; ++c) {
      pinMode(colPins_[c], INPUT_PULLUP);
    }
  }

  // Scans all rows/columns once; returns the raw index of the first active column found, or
  // keymatrix::NO_KEY if none are active.
  uint8_t scan() {
    for (uint8_t r = 0; r < keymatrix::ROWS; ++r) {
      digitalWrite(rowPins_[r], LOW);
      for (uint8_t c = 0; c < keymatrix::COLS; ++c) {
        if (digitalRead(colPins_[c]) == LOW) {
          digitalWrite(rowPins_[r], HIGH);
          return static_cast<uint8_t>(r * keymatrix::COLS + c);
        }
      }
      digitalWrite(rowPins_[r], HIGH);
    }
    return keymatrix::NO_KEY;
  }

 private:
  const uint8_t* rowPins_;
  const uint8_t* colPins_;
};

}  // namespace matrixscanner
