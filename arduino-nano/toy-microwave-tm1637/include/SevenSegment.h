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
// main loop; `periodFrames` is the full on+off cycle length. A generic on/off cycle a caller can
// apply to whatever it needs to signal -- this header has no opinion on which display state that
// is or what it should mean.
inline bool blinkOn(uint16_t frame, uint16_t periodFrames) {
  if (periodFrames == 0) return true;
  return (frame % periodFrames) < (periodFrames / 2);
}

// Segment byte for an ASCII character (bit0..6 = a..g, the standard convention this display's
// TM1637Display::encodeDigit() already uses for digits -- lives here rather than in an et6226m-
// style codec since this project has no such file; arduino-nano/toy-microwave-et6226m's
// equivalent uses the identical table under et6226m::encodeChar()). Case-insensitive by design:
// 'D' and 'd' return the same value, because a real seven-segment display has exactly one
// physical shape per letter -- there's no such thing as separate uppercase/lowercase fonts on the
// hardware itself, only whichever single rendering convention is most legible for that letter.
// Each letter below uses the industry-standard choice, which favors whichever case avoids
// colliding with a digit or another letter's shape where a clear alternative exists (e.g.
// lowercase b/d/h/k/n/q/r/t/v/y instead of capitals that would look like 8/0/H/H/N/O/R/T/V/V or
// similar). O is the one letter with no such alternative -- it's inherently the same round shape
// as digit 0, so it intentionally renders identically to it, same as virtually every 7-segment
// font. Covers space, 0-9, and A-Z; anything else (punctuation, control characters) returns 0x00
// (blank) rather than a garbled pattern. Table entries were derived by bit-reversing a commonly-
// circulated 7-segment font (which uses the opposite bit0=g...bit6=a convention) and cross-checked
// against this file's own independently-derived digit values.
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

}  // namespace sevenseg
