#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>

#include "../Song.h"

// Nokia Tune ("Grande Valse" excerpt, traditional/public domain -- popularized as Nokia's default
// ringtone). Transcribed from https://github.com/robsoncouto/arduino-songs (Robson Couto, 2019;
// no license stated there). PROGMEM -- see SongPlayer.h for why (small enough it would fit RAM
// too, but kept consistent with the rest of the borrowed-song catalog).
namespace nokia {

const song::Note NOTES[] PROGMEM = {

    // Nokia Ringtone
    // Score available at https://musescore.com/user/29944637/scores/5266155

    {notes::E5, 8}, {notes::D5, 8}, {notes::Fs4, 4}, {notes::Gs4, 4},
    {notes::Cs5, 8}, {notes::B4, 8}, {notes::D4, 4}, {notes::E4, 4},
    {notes::B4, 8}, {notes::A4, 8}, {notes::Cs4, 4}, {notes::E4, 4},
    {notes::A4, 2},
};
constexpr uint16_t TEMPO_BPM = 180;
constexpr uint16_t COUNT     = sizeof(NOTES) / sizeof(NOTES[0]);

}  // namespace nokia
