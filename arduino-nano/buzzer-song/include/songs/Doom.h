#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>

#include "../Song.h"

// "At Doom's Gate" (E1M1) -- Bobby Prince, id Software (1993). Personal, non-commercial hobby use
// only; copyright remains with id Software. Transcribed from
// https://github.com/robsoncouto/arduino-songs (Robson Couto, 2019; no license stated there).
// PROGMEM -- see SongPlayer.h for why.
namespace doom {

const song::Note NOTES[] PROGMEM = {

    // At Doom's Gate (E1M1)
    // Score available at https://musescore.com/pieridot/doom

    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::Fs3, -16}, {notes::D3, -16}, {notes::B2, -16}, {notes::A3, -16},
    {notes::Fs3, -16}, {notes::B2, -16}, {notes::D3, -16}, {notes::Fs3, -16}, {notes::A3, -16}, {notes::Fs3, -16}, {notes::D3, -16}, {notes::B2, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B3, -16}, {notes::G3, -16}, {notes::E3, -16}, {notes::G3, -16},
    {notes::B3, -16}, {notes::E4, -16}, {notes::G3, -16}, {notes::B3, -16}, {notes::E4, -16}, {notes::B3, -16}, {notes::G4, -16}, {notes::B4, -16},
    {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8},
    {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::Ds3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::E3, 8}, {notes::F3, 8},
    {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8},
    {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::Ds3, -2}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8},
    {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::Ds3, 8},
    {notes::A2, 8}, {notes::A2, 8}, {notes::E3, 8}, {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8},
    {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, -16}, {notes::F3, -16}, {notes::D3, -16}, {notes::A3, -16},
    {notes::F3, -16}, {notes::D3, -16}, {notes::C4, -16}, {notes::A3, -16}, {notes::F3, -16}, {notes::A3, -16}, {notes::F3, -16}, {notes::D3, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2},
    {notes::Cs3, 8}, {notes::Cs3, 8}, {notes::Cs4, 8}, {notes::Cs3, 8}, {notes::Cs3, 8}, {notes::B3, 8}, {notes::Cs3, 8}, {notes::Cs3, 8},
    {notes::A3, 8}, {notes::Cs3, 8}, {notes::Cs3, 8}, {notes::G3, 8}, {notes::Cs3, 8}, {notes::Cs3, 8}, {notes::Gs3, 8}, {notes::A3, 8},
    {notes::B2, 8}, {notes::B2, 8}, {notes::B3, 8}, {notes::B2, 8}, {notes::B2, 8}, {notes::A3, 8}, {notes::B2, 8}, {notes::B2, 8},
    {notes::G3, 8}, {notes::B2, 8}, {notes::B2, 8}, {notes::F3, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B3, -16}, {notes::G3, -16}, {notes::E3, -16}, {notes::G3, -16},
    {notes::B3, -16}, {notes::E4, -16}, {notes::G3, -16}, {notes::B3, -16}, {notes::E4, -16}, {notes::B3, -16}, {notes::G4, -16}, {notes::B4, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::Fs3, -16}, {notes::Ds3, -16}, {notes::B2, -16}, {notes::Fs3, -16},
    {notes::Ds3, -16}, {notes::B2, -16}, {notes::G3, -16}, {notes::D3, -16}, {notes::B2, -16}, {notes::Ds4, -16}, {notes::Ds3, -16}, {notes::B2, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E4, -16}, {notes::B3, -16}, {notes::G3, -16}, {notes::G4, -16},
    {notes::E4, -16}, {notes::G3, -16}, {notes::B3, -16}, {notes::D4, -16}, {notes::E4, -16}, {notes::G4, -16}, {notes::E4, -16}, {notes::G3, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8},
    {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::Ds3, 8},
    {notes::A2, 8}, {notes::A2, 8}, {notes::E3, 8}, {notes::F3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, 8}, {notes::A2, 8},
    {notes::A2, 8}, {notes::G3, 8}, {notes::A2, 8}, {notes::A2, 8}, {notes::A3, -16}, {notes::F3, -16}, {notes::D3, -16}, {notes::A3, -16},
    {notes::F3, -16}, {notes::D3, -16}, {notes::C4, -16}, {notes::A3, -16}, {notes::F3, -16}, {notes::A3, -16}, {notes::F3, -16}, {notes::D3, -16},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8},
    {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, -2}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::As2, 8},
    {notes::E2, 8}, {notes::E2, 8}, {notes::B2, 8}, {notes::C3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::E3, 8}, {notes::E2, 8},
    {notes::E2, 8}, {notes::D3, 8}, {notes::E2, 8}, {notes::E2, 8}, {notes::B3, -16}, {notes::G3, -16}, {notes::E3, -16}, {notes::B2, -16},
    {notes::E3, -16}, {notes::G3, -16}, {notes::C4, -16}, {notes::B3, -16}, {notes::G3, -16}, {notes::B3, -16}, {notes::G3, -16}, {notes::E3, -16},
};
constexpr uint16_t TEMPO_BPM = 225;
constexpr uint16_t COUNT     = sizeof(NOTES) / sizeof(NOTES[0]);

}  // namespace doom
