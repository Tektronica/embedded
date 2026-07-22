#pragma once

#include <stdint.h>

// 4x4 matrix keypad (rows driven one at a time, columns read back) — the standard membrane
// keypad layout (1-9, 0, *, #, A-D). Hardware-free: given which raw key index is active this
// scan (or NO_KEY), this header debounces it and reports the settled key character. main.cpp
// drives the actual row/column GPIO pins and maps the resulting key to a microwave::Event.
namespace keymatrix {

constexpr uint8_t ROWS = 4;
constexpr uint8_t COLS = 4;
constexpr uint8_t DEBOUNCE_SCANS = 3;   // consistent scans required before committing a keypress
constexpr uint8_t NO_KEY = 0xFF;        // sentinel raw index: no key currently pressed

// Standard 4x4 membrane keypad layout.
constexpr char LAYOUT[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

// Debounced key-press detector across the whole matrix — the same debounce pattern as
// nano/ledStripDimmer's Button class (N consistent samples before committing a state change),
// generalized from a boolean to a 16-way raw key index. Feed it the raw key index found by
// scanning all rows/columns once (or NO_KEY if none were active); returns the key character on a
// fresh, debounced press, or '\0' if nothing new settled this scan.
class Scanner {
 public:
  char scan(uint8_t rawKeyIndex) {
    if (rawKeyIndex == state_) {
      count_ = 0;
      return '\0';
    }
    if (++count_ < DEBOUNCE_SCANS) return '\0';
    count_ = 0;
    state_ = rawKeyIndex;
    if (state_ == NO_KEY) return '\0';
    return LAYOUT[state_ / COLS][state_ % COLS];
  }

 private:
  uint8_t state_ = NO_KEY;
  uint8_t count_ = 0;
};

}  // namespace keymatrix
