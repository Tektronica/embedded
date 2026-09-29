#pragma once

#include <stdint.h>

// Hardware-free 7-segment encoding and formatting. Segment bytes use the common bit0=A .. bit6=G,
// bit7=DP layout, which TM1637Display and the ET6226M both accept as-is.
namespace sevenseg {

//   -A-
//  F   B
//   -G-
//  E   C
//   -D-
// SEGBIT_* rather than SEG_*: TM1637Display.h defines SEG_A etc. as global macros.
constexpr uint8_t SEGBIT_A = 0x01, SEGBIT_B = 0x02, SEGBIT_C = 0x04, SEGBIT_D = 0x08,
                  SEGBIT_E = 0x10, SEGBIT_F = 0x20, SEGBIT_G = 0x40, SEGBIT_DP = 0x80;

constexpr uint8_t BLANK = 0x00;
constexpr uint8_t DASH  = SEGBIT_G;  // any character with no legible shape

// Digits 0-9 and the letters that are legible on seven segments, case-sensitive where the two
// cases have different shapes (H/h, U/u). Letters that would read as another letter or a digit
// (K, M, V, W, X, ...) return DASH rather than a misleading shape. Space is BLANK.
inline uint8_t encodeChar(char c) {
  switch (c) {
    case ' ': return BLANK;

    case '0': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F;
    case '1': return SEGBIT_B | SEGBIT_C;
    case '2': return SEGBIT_A | SEGBIT_B | SEGBIT_D | SEGBIT_E | SEGBIT_G;
    case '3': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_G;
    case '4': return SEGBIT_B | SEGBIT_C | SEGBIT_F | SEGBIT_G;
    case '5': return SEGBIT_A | SEGBIT_C | SEGBIT_D | SEGBIT_F | SEGBIT_G;
    case '6': return SEGBIT_A | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case '7': return SEGBIT_A | SEGBIT_B | SEGBIT_C;
    case '8': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case '9': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_F | SEGBIT_G;

    case 'A': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'b': return SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'C': return SEGBIT_A | SEGBIT_D | SEGBIT_E | SEGBIT_F;
    case 'd': return SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_G;
    case 'E': return SEGBIT_A | SEGBIT_D | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'F': return SEGBIT_A | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'G': return SEGBIT_A | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F;
    case 'H': return SEGBIT_B | SEGBIT_C | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'h': return SEGBIT_C | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'I': return SEGBIT_E | SEGBIT_F;
    case 'J': return SEGBIT_B | SEGBIT_C | SEGBIT_D;
    case 'L': return SEGBIT_D | SEGBIT_E | SEGBIT_F;
    case 'n': return SEGBIT_C | SEGBIT_E | SEGBIT_G;
    case 'o': return SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_G;
    case 'P': return SEGBIT_A | SEGBIT_B | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'q': return SEGBIT_A | SEGBIT_B | SEGBIT_C | SEGBIT_F | SEGBIT_G;
    case 'r': return SEGBIT_E | SEGBIT_G;
    case 'S': return SEGBIT_A | SEGBIT_C | SEGBIT_D | SEGBIT_F | SEGBIT_G;
    case 't': return SEGBIT_D | SEGBIT_E | SEGBIT_F | SEGBIT_G;
    case 'U': return SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_E | SEGBIT_F;
    case 'u': return SEGBIT_C | SEGBIT_D | SEGBIT_E;
    case 'y': return SEGBIT_B | SEGBIT_C | SEGBIT_D | SEGBIT_F | SEGBIT_G;
    case 'Z': return SEGBIT_A | SEGBIT_B | SEGBIT_D | SEGBIT_E | SEGBIT_G;

    default: return DASH;
  }
}

// Digit 0-9 -> segment byte; anything else is BLANK.
inline uint8_t encodeDigit(uint8_t digit) {
  return digit < 10 ? encodeChar(static_cast<char>('0' + digit)) : BLANK;
}

// Up to `count` characters of `text`, left to right, BLANK-padded when `text` is shorter.
// Alignment is the caller's: pad `text` with spaces (e.g. " End").
inline void encodeText(const char* text, uint8_t* outSegments, uint8_t count) {
  uint8_t i = 0;
  for (; i < count && text[i] != '\0'; ++i) outSegments[i] = encodeChar(text[i]);
  for (; i < count; ++i) outSegments[i] = BLANK;
}

// On for the first half of each `period`-long cycle of `frame` (any unit: loop frames, ms).
inline bool blinkOn(uint16_t frame, uint16_t period) {
  if (period == 0) return true;
  return (frame % period) < (period / 2);
}

constexpr uint16_t MAX_SECONDS = 99 * 60 + 59;  // 99:59, the 2-digit-minutes ceiling

// MM:SS digit values, most significant first.
struct Digits {
  uint8_t minutesTens;
  uint8_t minutesOnes;
  uint8_t secondsTens;
  uint8_t secondsOnes;
};

// Seconds -> MM:SS digits, clamped to 99:59 rather than wrapping to a wrong time.
inline Digits secondsToDigits(uint16_t totalSeconds) {
  if (totalSeconds > MAX_SECONDS) totalSeconds = MAX_SECONDS;
  uint8_t minutes = static_cast<uint8_t>(totalSeconds / 60);
  uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60);
  return Digits{static_cast<uint8_t>(minutes / 10), static_cast<uint8_t>(minutes % 10),
                static_cast<uint8_t>(seconds / 10), static_cast<uint8_t>(seconds % 10)};
}

}  // namespace sevenseg
