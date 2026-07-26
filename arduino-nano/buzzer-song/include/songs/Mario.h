#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>

#include "Song.h"

// Super Mario Bros. "Overworld" theme -- Koji Kondo, Nintendo (1985). Personal, non-commercial
// hobby use only; copyright remains with Nintendo. Transcribed from
// https://github.com/robsoncouto/arduino-songs (no license stated there). PROGMEM -- see
// SongPlayer.h for why.
namespace mario {

const song::Note NOTES[] PROGMEM = {

    // Super Mario Bros theme
    // Score available at https://musescore.com/user/2123/scores/2145
    // Theme by Koji Kondo

    {notes::E5, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::E5, 8}, //1
    {notes::G5, 4}, {notes::REST, 4}, {notes::G4, 8}, {notes::REST, 4},
    {notes::C5, -4}, {notes::G4, 8}, {notes::REST, 4}, {notes::E4, -4}, // 3
    {notes::A4, 4}, {notes::B4, 4}, {notes::As4, 8}, {notes::A4, 4},
    {notes::G4, -8}, {notes::E5, -8}, {notes::G5, -8}, {notes::A5, 4}, {notes::F5, 8}, {notes::G5, 8},
    {notes::REST, 8}, {notes::E5, 4},{notes::C5, 8}, {notes::D5, 8}, {notes::B4, -4},
    {notes::C5, -4}, {notes::G4, 8}, {notes::REST, 4}, {notes::E4, -4}, // repeats from 3
    {notes::A4, 4}, {notes::B4, 4}, {notes::As4, 8}, {notes::A4, 4},
    {notes::G4, -8}, {notes::E5, -8}, {notes::G5, -8}, {notes::A5, 4}, {notes::F5, 8}, {notes::G5, 8},
    {notes::REST, 8}, {notes::E5, 4},{notes::C5, 8}, {notes::D5, 8}, {notes::B4, -4},

    {notes::REST, 4}, {notes::G5, 8}, {notes::Fs5, 8}, {notes::F5, 8}, {notes::Ds5, 4}, {notes::E5, 8},//7
    {notes::REST, 8}, {notes::Gs4, 8}, {notes::A4, 8}, {notes::C4, 8}, {notes::REST, 8}, {notes::A4, 8}, {notes::C5, 8}, {notes::D5, 8},
    {notes::REST, 4}, {notes::Ds5, 4}, {notes::REST, 8}, {notes::D5, -4},
    {notes::C5, 2}, {notes::REST, 2},

    {notes::REST, 4}, {notes::G5, 8}, {notes::Fs5, 8}, {notes::F5, 8}, {notes::Ds5, 4}, {notes::E5, 8},//repeats from 7
    {notes::REST, 8}, {notes::Gs4, 8}, {notes::A4, 8}, {notes::C4, 8}, {notes::REST, 8}, {notes::A4, 8}, {notes::C5, 8}, {notes::D5, 8},
    {notes::REST, 4}, {notes::Ds5, 4}, {notes::REST, 8}, {notes::D5, -4},
    {notes::C5, 2}, {notes::REST, 2},

    {notes::C5, 8}, {notes::C5, 4}, {notes::C5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::D5, 4},//11
    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2},

    {notes::C5, 8}, {notes::C5, 4}, {notes::C5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::D5, 8}, {notes::E5, 8},//13
    {notes::REST, 1},
    {notes::C5, 8}, {notes::C5, 4}, {notes::C5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::D5, 4},
    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2},
    {notes::E5, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::E5, 4},
    {notes::G5, 4}, {notes::REST, 4}, {notes::G4, 4}, {notes::REST, 4},
    {notes::C5, -4}, {notes::G4, 8}, {notes::REST, 4}, {notes::E4, -4}, // 19

    {notes::A4, 4}, {notes::B4, 4}, {notes::As4, 8}, {notes::A4, 4},
    {notes::G4, -8}, {notes::E5, -8}, {notes::G5, -8}, {notes::A5, 4}, {notes::F5, 8}, {notes::G5, 8},
    {notes::REST, 8}, {notes::E5, 4}, {notes::C5, 8}, {notes::D5, 8}, {notes::B4, -4},

    {notes::C5, -4}, {notes::G4, 8}, {notes::REST, 4}, {notes::E4, -4}, // repeats from 19
    {notes::A4, 4}, {notes::B4, 4}, {notes::As4, 8}, {notes::A4, 4},
    {notes::G4, -8}, {notes::E5, -8}, {notes::G5, -8}, {notes::A5, 4}, {notes::F5, 8}, {notes::G5, 8},
    {notes::REST, 8}, {notes::E5, 4}, {notes::C5, 8}, {notes::D5, 8}, {notes::B4, -4},

    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},//23
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::D5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::G5, -8}, {notes::F5, -8},

    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2}, //26
    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::B4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::F5, -8}, {notes::E5, -8}, {notes::D5, -8},
    {notes::C5, 8}, {notes::E4, 4}, {notes::E4, 8}, {notes::C4, 2},

    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},//repeats from 23
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::D5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::G5, -8}, {notes::F5, -8},

    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2}, //26
    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::B4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::F5, -8}, {notes::E5, -8}, {notes::D5, -8},
    {notes::C5, 8}, {notes::E4, 4}, {notes::E4, 8}, {notes::C4, 2},
    {notes::C5, 8}, {notes::C5, 4}, {notes::C5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::D5, 8}, {notes::E5, 8},
    {notes::REST, 1},

    {notes::C5, 8}, {notes::C5, 4}, {notes::C5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::D5, 4}, //33
    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2},
    {notes::E5, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::E5, 8}, {notes::REST, 8}, {notes::C5, 8}, {notes::E5, 4},
    {notes::G5, 4}, {notes::REST, 4}, {notes::G4, 4}, {notes::REST, 4},
    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::D5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::A5, -8}, {notes::G5, -8}, {notes::F5, -8},

    {notes::E5, 8}, {notes::C5, 4}, {notes::A4, 8}, {notes::G4, 2}, //40
    {notes::E5, 8}, {notes::C5, 4}, {notes::G4, 8}, {notes::REST, 4}, {notes::Gs4, 4},
    {notes::A4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::A4, 2},
    {notes::B4, 8}, {notes::F5, 4}, {notes::F5, 8}, {notes::F5, -8}, {notes::E5, -8}, {notes::D5, -8},
    {notes::C5, 8}, {notes::E4, 4}, {notes::E4, 8}, {notes::C4, 2},

    //game over sound
    {notes::C5, -4}, {notes::G4, -4}, {notes::E4, 4}, //45
    {notes::A4, -8}, {notes::B4, -8}, {notes::A4, -8}, {notes::Gs4, -8}, {notes::As4, -8}, {notes::Gs4, -8},
    {notes::G4, 8}, {notes::D4, 8}, {notes::E4, -2},
};
constexpr uint16_t TEMPO_BPM = 200;
constexpr uint16_t COUNT     = sizeof(NOTES) / sizeof(NOTES[0]);

}  // namespace mario
