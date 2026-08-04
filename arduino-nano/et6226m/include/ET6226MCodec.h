#pragma once

#include <stdint.h>

// Pure, hardware-free helpers for the UMW ET6226M's display/keyboard data -- unit-tested off-
// device (pio test -e native), separate from ET6226M.h's driver class (which needs real CLK/DAT
// pins and includes Arduino.h) so a test file that only wants this pure logic doesn't have to
// pull in Arduino.h too.
namespace et6226m {

// Segment byte (bit6..0 = SG7..SG1, no DP) for a digit 0-9. Assumes SG1=a, SG2=b, ... SG7=g, the
// common convention -- not confirmed against this project's actual display board wiring yet (see
// README.md's "Open questions"). Our own table, not shared with TM1637Display's, since we're not
// depending on that library for this chip.
inline uint8_t encodeDigit(uint8_t digit) {
  static constexpr uint8_t SEGMENTS[10] = {
      0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
  };
  return digit < 10 ? SEGMENTS[digit] : 0x00;
}

constexpr uint8_t MAX_BRIGHTNESS = 7;  // 0 = dimmest (datasheet's "1 step"), 7 = brightest ("8 step")

// Builds the Display Control Command's data byte (sent after command 0x48): D6-D4 = brightness
// (0-7, clamped), D3 = 1 to fix this driver to 7-segment mode -- matching a 4-digit 7-segment
// display, this project's target -- rather than the datasheet's alternative 8-segment mode,
// which repurposes DP/KP as an extra segment output instead of its keyboard-scan role. Sleep
// mode (D2) isn't exposed yet -- see README.md's "Open questions". Reconstructed from the
// datasheet's own worked examples ("X1H" = 8-segment mode, "X9H" = 7-segment mode, "04H" =
// sleep mode, and "D0 and D2 cannot be 1 at the same time").
inline uint8_t encodeDisplayControl(uint8_t brightness, bool displayOn) {
  uint8_t clamped = brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness;
  return static_cast<uint8_t>((clamped << 4) | 0x08 | (displayOn ? 0x01 : 0x00));
}

// A scanned key position: 1-based grid (1-4) and segment (1-7); {0, 0} means no key.
struct KeyPosition {
  uint8_t grid;
  uint8_t segment;
};

// Reverses the datasheet's Key Code Command table: each (segment, grid) pair maps to
// 0x44 + (segment-1)*8 + (grid-1). Out-of-range or zero codes decode to "no key" rather than
// guessing at a position, since a garbled read shouldn't be reported as a real keypress.
inline KeyPosition decodeKeyCode(uint8_t rawCode) {
  if (rawCode < 0x44) return KeyPosition{0, 0};
  uint8_t offset = static_cast<uint8_t>(rawCode - 0x44);
  uint8_t segmentIndex0 = offset / 8;
  uint8_t gridIndex0 = offset % 8;
  if (segmentIndex0 >= 7 || gridIndex0 >= 4) return KeyPosition{0, 0};
  return KeyPosition{static_cast<uint8_t>(gridIndex0 + 1), static_cast<uint8_t>(segmentIndex0 + 1)};
}

}  // namespace et6226m
