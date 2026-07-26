#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>

#include "Song.h"
#include "songs/Doom.h"
#include "songs/Mario.h"
#include "songs/Nokia.h"
#include "songs/Tetris.h"

// The generic playback engine for PROGMEM-resident songs, plus the single production entry
// point main.cpp calls regardless of which track is selected. Each song's own note table lives in
// its own header under songs/ (Tetris.h, Mario.h, ...) -- adding a song means a new header + one
// enum value + one switch case here, never a change to the engine itself.
//
// PROGMEM: AVR copies ordinary `const` globals from flash into RAM at boot regardless of
// constness; only PROGMEM keeps them flash-resident, read back via the LPM instruction
// (`pgm_read_*`). Tetris (97 notes), Mario (321 notes), and Doom (680 notes) don't fit the
// ATmega328P's 2KB SRAM as plain arrays, hence PROGMEM. That's also why this walk can't live in
// Song.h's pure, native-testable toneStateFor(): avr/pgmspace.h doesn't exist off-device. Scale is
// a small, original track and stays RAM-resident, walked directly by Song.h's toneStateFor().
namespace songplayer {

namespace detail {

inline song::Note readProgmemNote(const song::Note* notes, uint16_t index) {
  song::Note note;
  note.frequencyHz     = pgm_read_word_near(&notes[index].frequencyHz);
  note.durationDivider = static_cast<int8_t>(pgm_read_byte_near(&notes[index].durationDivider));
  return note;
}

}  // namespace detail

// Same algorithm as song::toneStateFor(), adapted to read PROGMEM-resident notes one at a time
// instead of indexing a RAM array directly.
inline song::ToneState toneStateForProgmem(const song::Note* notes, uint16_t count,
                                            uint16_t tempoBpm, uint32_t elapsedMs) {
  uint32_t totalMs = 0;
  for (uint16_t i = 0; i < count; ++i) {
    totalMs += song::durationMs(detail::readProgmemNote(notes, i).durationDivider, tempoBpm);
  }
  if (totalMs == 0) return song::ToneState{false, notes::REST};  // empty/zero-duration track

  uint32_t positionMs = elapsedMs % totalMs;
  for (uint16_t i = 0; i < count; ++i) {
    song::Note note   = detail::readProgmemNote(notes, i);
    uint16_t   noteMs = song::durationMs(note.durationDivider, tempoBpm);
    if (positionMs < noteMs) {
      uint16_t onMs = static_cast<uint16_t>(static_cast<uint32_t>(noteMs) *
                                             song::NOTE_ON_PERCENT / 100);
      bool on = note.frequencyHz != notes::REST && positionMs < onMs;
      return song::ToneState{on, note.frequencyHz};
    }
    positionMs -= noteMs;
  }
  return song::ToneState{false, notes::REST};  // unreachable: positionMs < totalMs by construction
}

// Single production entry point: what the buzzer should be doing right now for `currentTrack`,
// `elapsedMs` into its loop. Scale is RAM-resident and walked by song::toneStateFor();
// Tetris/Mario/Doom/Nokia are PROGMEM-resident and walked by toneStateForProgmem() above.
inline song::ToneState update(song::Track currentTrack, uint32_t elapsedMs) {
  switch (currentTrack) {
    case song::Track::Tetris:
      return toneStateForProgmem(tetris::NOTES, tetris::COUNT, tetris::TEMPO_BPM, elapsedMs);
    case song::Track::Mario:
      return toneStateForProgmem(mario::NOTES, mario::COUNT, mario::TEMPO_BPM, elapsedMs);
    case song::Track::Doom:
      return toneStateForProgmem(doom::NOTES, doom::COUNT, doom::TEMPO_BPM, elapsedMs);
    case song::Track::Nokia:
      return toneStateForProgmem(nokia::NOTES, nokia::COUNT, nokia::TEMPO_BPM, elapsedMs);
    case song::Track::Scale:
    default:
      return song::toneStateFor(song::SCALE, song::SCALE_COUNT, song::SCALE_TEMPO_BPM, elapsedMs);
  }
}

}  // namespace songplayer
