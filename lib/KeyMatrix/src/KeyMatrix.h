#pragma once

#include <stdint.h>
#include <Arduino.h>

// Row/column key matrix scan driver: drives each row low in turn and reads back which column is
// pulled low. Returns a raw index (row * colCount + col) or NO_KEY. Debouncing and the key layout
// belong to the app.
namespace keymatrix {

constexpr uint8_t NO_KEY = 0xFF;

class MatrixScanner {
 public:
  MatrixScanner(const uint8_t* rowPins, uint8_t rowCount, const uint8_t* colPins, uint8_t colCount)
      : rowPins_(rowPins), colPins_(colPins), rowCount_(rowCount), colCount_(colCount) {}

  void begin() {
    for (uint8_t r = 0; r < rowCount_; ++r) {
      pinMode(rowPins_[r], OUTPUT);
      digitalWrite(rowPins_[r], HIGH);
    }
    for (uint8_t c = 0; c < colCount_; ++c) {
      pinMode(colPins_[c], INPUT_PULLUP);
    }
  }

  // One full scan; the first active key found wins.
  uint8_t scan() {
    for (uint8_t r = 0; r < rowCount_; ++r) {
      digitalWrite(rowPins_[r], LOW);
      for (uint8_t c = 0; c < colCount_; ++c) {
        if (digitalRead(colPins_[c]) == LOW) {
          digitalWrite(rowPins_[r], HIGH);
          return static_cast<uint8_t>(r * colCount_ + c);
        }
      }
      digitalWrite(rowPins_[r], HIGH);
    }
    return NO_KEY;
  }

 private:
  const uint8_t* rowPins_;
  const uint8_t* colPins_;
  uint8_t rowCount_;
  uint8_t colCount_;
};

}  // namespace keymatrix
