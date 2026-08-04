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

// Segment byte for an ASCII character (bit0..6 = a..g, same convention as encodeDigit() and
// electrically confirmed for this chip -- see README.md). Case-insensitive by design: 'D' and
// 'd' return the same value, because a real seven-segment display has exactly one physical shape
// per letter -- there's no such thing as separate uppercase/lowercase fonts on the hardware
// itself, only whichever single rendering convention is most legible for that letter. Each
// letter below uses the industry-standard choice, which favors whichever case avoids colliding
// with a digit or another letter's shape where a clear alternative exists (e.g. lowercase
// b/d/h/k/n/q/r/t/v/y instead of capitals that would look like 8/0/H/H/N/O/R/T/V/V or similar).
// O is the one letter with no such alternative -- it's inherently the same round shape as digit
// 0, so it intentionally renders identically to it, same as virtually every 7-segment font.
// Covers space, 0-9, and A-Z; anything else (punctuation, control characters) returns 0x00
// (blank) rather than a garbled pattern. Table entries were derived by bit-reversing a commonly-
// circulated 7-segment font (which uses the opposite bit0=g...bit6=a convention) and cross-
// checked against this file's own independently-derived digit values.
inline uint8_t encodeChar(char c) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));  // normalize to uppercase
  if (c < '0' || c > 'Z') return 0x00;
  static constexpr uint8_t TABLE[] = {
      0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // '0'-'9'
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                    // ':'-'@', unused
      // 'A'-'Z', one canonical value per letter regardless of input case
      0x77,  // A
      0x7C,  // B -> lowercase-style b (capital B collides with 8)
      0x39,  // C
      0x5E,  // D -> lowercase-style d (capital D collides with 0)
      0x79,  // E
      0x71,  // F
      0x3D,  // G
      0x74,  // H -> lowercase-style h
      0x06,  // I (same shape as digit 1, the accepted standard)
      0x1E,  // J
      0x74,  // K -> lowercase-style k (shares H's shape on seven segments)
      0x38,  // L
      0x55,  // M
      0x54,  // N
      0x3F,  // O (collides with 0 -- accepted; O has no distinct alternative, unlike D)
      0x73,  // P
      0x67,  // Q -> lowercase-style q
      0x50,  // R -> lowercase-style r
      0x6D,  // S
      0x78,  // T -> lowercase-style t
      0x3E,  // U
      0x62,  // V -> lowercase-style v
      0x7E,  // W (a true W doesn't fit one seven-segment digit -- this is an approximation)
      0x52,  // X
      0x66,  // Y -> lowercase-style y
      0x5B,  // Z
  };
  return TABLE[static_cast<uint8_t>(c) - '0'];
}

// Encodes up to `count` characters of `text` into `outSegments`, left to right, blanking any
// trailing positions if `text` is shorter than `count`. Doesn't wrap, scroll, or otherwise handle
// text longer than `count` -- it simply stops -- since nothing needs that yet; a caller wanting a
// specific alignment (e.g. right-aligned, like " End") controls it with literal leading/trailing
// spaces in `text` rather than this function taking an alignment option.
inline void encodeText(const char* text, uint8_t* outSegments, uint8_t count) {
  uint8_t i = 0;
  for (; i < count && text[i] != '\0'; ++i) outSegments[i] = encodeChar(text[i]);
  for (; i < count; ++i) outSegments[i] = 0x00;
}

constexpr uint8_t MAX_BRIGHTNESS = 7;  // 0 = dimmest (datasheet's "1 step"), 7 = brightest ("8 step")

// D3 of the Display Control Command: whether DP/KP acts as an 8th segment output (EightSegment)
// or is freed for its KP role instead (SevenSegment). This is a property of how a specific board
// wires DP/KP -- e.g. tied to a display's shared DP line, or left for keyboard use -- not a
// runtime preference, so callers pick one at construction (see ET6226M's constructor) rather
// than switching modes live.
enum class SegmentMode : uint8_t { EightSegment = 0x00, SevenSegment = 0x08 };

// Builds the Display Control Command's data byte (sent after command 0x48): D6-D4 = brightness
// (0-7, clamped), D3 = the given SegmentMode, D0 = display on/off. Sleep mode (D2) isn't exposed
// yet -- see README.md's "Open questions". Reconstructed from the datasheet's own worked examples
// ("X1H" = 8-segment mode, "X9H" = 7-segment mode, "04H" = sleep mode, and "D0 and D2 cannot be 1
// at the same time").
inline uint8_t encodeDisplayControl(uint8_t brightness, bool displayOn, SegmentMode mode) {
  uint8_t clamped = brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness;
  return static_cast<uint8_t>((clamped << 4) | static_cast<uint8_t>(mode) |
                               (displayOn ? 0x01 : 0x00));
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
