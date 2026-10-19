#pragma once

#include <stdint.h>

#include "SevenSeg.h"

// 4-digit 7-segment display (TM1637 driver board, same part toy-microwave uses). Hardware-free
// (no Arduino.h, no TM1637 library dependency) so it unit-tests off-device; main.cpp hands
// render()'s output straight to TM1637Display::setSegments().
//
// Design: `Content` (what to show -- a number or a label) and `Mode` (how to show it -- static,
// flashing, or rolling) are independent types. A developer sets one without knowing the other
// exists; render() is the single place they're composed. Same shape as buzzer-song's
// Note/Track-vs-SongPlayer split and led-dimmer-ws2812's pixelColor(): one small render
// primitive, gated by an orthogonal mode, rather than a class per mode. A `Mode` enum + one
// switch inside render() is deliberately preferred over a Strategy-pattern class hierarchy here
// -- the mode set is small and fixed at compile time, so virtual dispatch would only cost flash
// and indirection for no real flexibility gained.
namespace segdisplay {

// What to show -- built by numberContent()/labelContent() rather than aggregate-initialized
// directly, so a caller never has to know which fields a given ContentType actually uses.
enum class ContentType : uint8_t { Number, Label };

struct Content {
  ContentType type;
  uint16_t    number;       // valid when type == Number; clamped to 0-9999 (4 digits)
  const char* label;        // valid when type == Label
  uint8_t     labelLength;
};

inline Content numberContent(uint16_t number) {
  return Content{ContentType::Number, number, nullptr, 0};
}

inline Content labelContent(const char* label, uint8_t labelLength) {
  return Content{ContentType::Label, 0, label, labelLength};
}

// How to show it, independent of what it is.
enum class Mode : uint8_t { Static, Flashing, Rolling };

constexpr uint8_t WINDOW = 4;  // this display's digit count

// Which position in a scrolling label's cycle `frame` falls on -- one step every
// `framesPerStep` frames. The cycle is `labelLength + WINDOW` long, so a full WINDOW's worth of
// blank space passes before the label repeats, rather than immediately wrapping into itself.
inline uint16_t rollOffset(uint8_t labelLength, uint16_t frame, uint16_t framesPerStep) {
  if (framesPerStep == 0) framesPerStep = 1;
  uint16_t cycleLen = static_cast<uint16_t>(labelLength) + WINDOW;
  return (frame / framesPerStep) % cycleLen;
}

struct Segments {
  uint8_t values[WINDOW];
};

namespace detail {

// Content shown verbatim, left-justified, blank-padded -- what Static shows, and what Flashing
// blinks between showing and blanking.
inline Segments renderContent(const Content& content) {
  Segments out{};
  if (content.type == ContentType::Number) {
    uint16_t n = content.number > 9999 ? 9999 : content.number;
    out.values[0] = sevenseg::encodeChar(static_cast<char>('0' + (n / 1000) % 10));
    out.values[1] = sevenseg::encodeChar(static_cast<char>('0' + (n / 100) % 10));
    out.values[2] = sevenseg::encodeChar(static_cast<char>('0' + (n / 10) % 10));
    out.values[3] = sevenseg::encodeChar(static_cast<char>('0' + n % 10));
  } else {
    for (uint8_t i = 0; i < WINDOW; ++i) {
      char c = (i < content.labelLength) ? content.label[i] : ' ';
      out.values[i] = sevenseg::encodeChar(c);
    }
  }
  return out;
}

}  // namespace detail

// The single render primitive: what four segment bytes to show right now, for `content` under
// `mode`, `frame` ticks since `mode` was last (re)started. `periodFrames` means the blink period
// under Flashing, or frames-per-scroll-step under Rolling; unused under Static. Rolling only
// applies to Label content -- a Number is always within the 4-digit window already, so it falls
// back to the same verbatim rendering Static uses.
inline Segments render(const Content& content, Mode mode, uint16_t frame, uint16_t periodFrames) {
  if (mode == Mode::Rolling && content.type == ContentType::Label) {
    Segments out{};
    uint16_t cycleLen = static_cast<uint16_t>(content.labelLength) + WINDOW;
    uint16_t offset   = rollOffset(content.labelLength, frame, periodFrames);
    for (uint8_t i = 0; i < WINDOW; ++i) {
      uint16_t idx = (offset + i) % cycleLen;
      char     c   = (idx < content.labelLength) ? content.label[idx] : ' ';
      out.values[i] = sevenseg::encodeChar(c);
    }
    return out;
  }

  Segments out = detail::renderContent(content);
  if (mode == Mode::Flashing && !sevenseg::blinkOn(frame, periodFrames)) {
    for (uint8_t i = 0; i < WINDOW; ++i) out.values[i] = sevenseg::BLANK;
  }
  return out;
}

// ORs a colon/decimal-point segment into one digit of an already-rendered Segments, if
// `colonOn` -- composable with any Mode's output, since a colon is a decorative segment on one
// specific digit, not a value-presentation strategy of its own. Deliberately not a fourth Mode
// value: that would make colon-blink mutually exclusive with Flashing/Rolling, when a real
// clock shows a static or rolling value with an independently blinking colon. Drive `colonOn`
// with the same blinkOn() used for Flashing, on whatever schedule the caller wants.
//
// `colonDigitIndex` and which bit is actually wired to the colon both vary by board --
// sevenseg::SEGBIT_DP is the common one, but confirm against your specific display.
inline Segments withColon(Segments segments, uint8_t colonDigitIndex, bool colonOn) {
  if (colonOn && colonDigitIndex < WINDOW) segments.values[colonDigitIndex] |= sevenseg::SEGBIT_DP;
  return segments;
}

}  // namespace segdisplay
