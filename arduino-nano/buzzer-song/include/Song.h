#pragma once

#include <stdint.h>

#include "Notes.h"

// Pure music-timing toolkit: note/duration types and the tempo math to turn them into
// milliseconds, plus the one small, original looping track (Scale). Hardware-free (no
// Arduino.h) so it unit-tests off-device; main.cpp/SongPlayer.h map ToneState to tone()/noTone()
// calls. Same ToneState shape as toy-microwave's and game-dino-run's Buzzer.h.
//
// Larger, borrowed songs (Tetris, Mario, Doom -- see songs/) reuse durationMs() for their timing
// math but store their note tables PROGMEM and walk them separately, since reading PROGMEM needs
// avr/pgmspace.h, which doesn't exist off-device.
namespace song {

// A single note: frequency in Hz (REST = silence) and a duration divider using the classic
// tracker/ringtone convention -- 4=quarter, 8=eighth, 16=sixteenth, 2=half, 1=whole note;
// negative = dotted (that duration plus half again), e.g. -4 = dotted quarter.
struct Note {
  uint16_t frequencyHz;
  int8_t   durationDivider;
};

enum class Track : uint8_t { Scale, Tetris, Mario, Doom, Nokia };

inline Track next(Track current) {
  return static_cast<Track>((static_cast<uint8_t>(current) + 1) % 5);
}

// Duration of a whole note (4 beats) at a given tempo, in ms.
constexpr uint32_t wholeNoteMs(uint16_t tempoBpm) {
  return (60000UL * 4) / tempoBpm;
}

// Duration of one note at a given tempo, in ms. Dotted notes (negative divider) run 1.5x the
// plain division -- see Note's comment. 0 isn't a valid divider (never appears in any song's
// data); guarded here rather than at each call site since this is the one place that would
// otherwise divide by it.
inline uint16_t durationMs(int8_t divider, uint16_t tempoBpm) {
  if (divider == 0) return 0;
  uint32_t whole = wholeNoteMs(tempoBpm);
  if (divider > 0) return static_cast<uint16_t>(whole / divider);
  uint32_t base = whole / -divider;
  return static_cast<uint16_t>(base + base / 2);
}

// Percentage of a note's duration actually sounded -- the rest is silence, so consecutive
// same-pitch notes (or a note followed by an identical one) are heard as separate notes rather
// than one continuous tone. Matches the 90/10 split this song data was originally authored with.
constexpr uint8_t NOTE_ON_PERCENT = 90;

// Whether the buzzer should be sounding right now, and at what frequency, `elapsedMs` into a
// looping track.
struct ToneState {
  bool     on;
  uint16_t frequencyHz;
};

// Loops a RAM-resident `notes` array forever; `elapsedMs` is the position since the loop started.
inline ToneState toneStateFor(const Note* notes, uint8_t count, uint16_t tempoBpm,
                               uint32_t elapsedMs) {
  uint32_t totalMs = 0;
  for (uint8_t i = 0; i < count; ++i) totalMs += durationMs(notes[i].durationDivider, tempoBpm);
  if (totalMs == 0) return ToneState{false, notes::REST};  // empty/zero-duration track

  uint32_t positionMs = elapsedMs % totalMs;
  for (uint8_t i = 0; i < count; ++i) {
    uint16_t noteMs = durationMs(notes[i].durationDivider, tempoBpm);
    if (positionMs < noteMs) {
      uint16_t onMs = static_cast<uint16_t>(static_cast<uint32_t>(noteMs) * NOTE_ON_PERCENT / 100);
      bool on = notes[i].frequencyHz != notes::REST && positionMs < onMs;
      return ToneState{on, notes[i].frequencyHz};
    }
    positionMs -= noteMs;
  }
  return ToneState{false, notes::REST};  // unreachable: positionMs < totalMs by construction
}

// Do, re, mi, fa, so, la, ti, do -- one octave up and looping. Quarter notes at 200 BPM.
constexpr Note SCALE[] = {
    {notes::C4, 4}, {notes::D4, 4}, {notes::E4, 4}, {notes::F4, 4},
    {notes::G4, 4}, {notes::A4, 4}, {notes::B4, 4}, {notes::C5, 4},
};
constexpr uint16_t SCALE_TEMPO_BPM = 200;
constexpr uint8_t  SCALE_COUNT     = sizeof(SCALE) / sizeof(SCALE[0]);

}  // namespace song
