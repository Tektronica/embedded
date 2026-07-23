#pragma once

#include <stdint.h>

// 4-digit 7-segment display (TM1637 driver board, 2-wire CLK/DIO protocol). The TM1637 chip
// handles digit -> segment-pattern encoding and multiplexing itself, via a library, so this
// header's job is countdown-time formatting instead: seconds -> the four digit values (and any
// blink/colon state) to hand to that library. Hardware-free so it unit-tests off-device.
namespace sevenseg {

constexpr uint16_t MAX_SECONDS = 99 * 60 + 59;  // 99:59, this display's 2-digit-minutes ceiling

// The four digit values (MM:SS, most significant first) plus whether the colon should be lit.
struct Digits {
  uint8_t minutesTens;
  uint8_t minutesOnes;
  uint8_t secondsTens;
  uint8_t secondsOnes;
  bool colonOn;
};

// Format a countdown in seconds as MM:SS digit values. Clamps to this display's 99:59 ceiling
// rather than wrapping, since a wrapped value would silently show the wrong time.
inline Digits secondsToDigits(uint16_t totalSeconds) {
  if (totalSeconds > MAX_SECONDS) totalSeconds = MAX_SECONDS;
  uint8_t minutes = static_cast<uint8_t>(totalSeconds / 60);
  uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60);
  return Digits{static_cast<uint8_t>(minutes / 10), static_cast<uint8_t>(minutes % 10),
                static_cast<uint8_t>(seconds / 10), static_cast<uint8_t>(seconds % 10), true};
}

// Is the display "on" for this frame of a blink cycle? `frame` increments once per call from the
// main loop; `periodFrames` is the full on+off cycle length. Used for the colon while Setting and
// the whole display while Done, so the panel doesn't just sit static in those states.
inline bool blinkOn(uint16_t frame, uint16_t periodFrames) {
  if (periodFrames == 0) return true;
  return (frame % periodFrames) < (periodFrames / 2);
}

}  // namespace sevenseg
