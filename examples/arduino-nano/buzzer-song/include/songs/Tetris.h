#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>

#include "Song.h"

// Tetris theme ("Korobeiniki") -- traditional Russian folk melody (public domain). Arrangement
// transcribed from https://github.com/robsoncouto/arduino-songs (Robson Couto, 2019). PROGMEM --
// see SongPlayer.h for why.
namespace tetris {

const song::Note NOTES[] PROGMEM = {

    //Based on the arrangement at https://www.flutetunes.com/tunes.php?id=192

    {notes::E5, 4},  {notes::B4, 8},  {notes::C5, 8},  {notes::D5, 4},  {notes::C5, 8},  {notes::B4, 8},
    {notes::A4, 4},  {notes::A4, 8},  {notes::C5, 8},  {notes::E5, 4},  {notes::D5, 8},  {notes::C5, 8},
    {notes::B4, -4},  {notes::C5, 8},  {notes::D5, 4},  {notes::E5, 4},
    {notes::C5, 4},  {notes::A4, 4},  {notes::A4, 4}, {notes::REST, 4},

    {notes::REST, 8}, {notes::D5, 4},  {notes::F5, 8},  {notes::A5, 4},  {notes::G5, 8},  {notes::F5, 8},
    {notes::E5, -4},  {notes::C5, 8},  {notes::E5, 4},  {notes::D5, 8},  {notes::C5, 8},
    {notes::B4, 4},  {notes::B4, 8},  {notes::C5, 8},  {notes::D5, 4},  {notes::E5, 4},
    {notes::C5, 4},  {notes::A4, 4},  {notes::A4, 4}, {notes::REST, 4},

    {notes::E5, 2}, {notes::C5, 2},
    {notes::D5, 2}, {notes::B4, 2},
    {notes::C5, 2}, {notes::A4, 2},
    {notes::B4, 1},

    {notes::E5, 2}, {notes::C5, 2},
    {notes::D5, 2}, {notes::B4, 2},
    {notes::C5, 4}, {notes::E5, 4}, {notes::A5, 2},
    {notes::Gs5, 1},

    {notes::E5, 4},  {notes::B4, 8},  {notes::C5, 8},  {notes::D5, 4},  {notes::C5, 8},  {notes::B4, 8},
    {notes::A4, 4},  {notes::A4, 8},  {notes::C5, 8},  {notes::E5, 4},  {notes::D5, 8},  {notes::C5, 8},
    {notes::B4, -4},  {notes::C5, 8},  {notes::D5, 4},  {notes::E5, 4},
    {notes::C5, 4},  {notes::A4, 4},  {notes::A4, 4}, {notes::REST, 4},

    {notes::REST, 8}, {notes::D5, 4},  {notes::F5, 8},  {notes::A5, 4},  {notes::G5, 8},  {notes::F5, 8},
    {notes::REST, 8}, {notes::E5, 4},  {notes::C5, 8},  {notes::E5, 4},  {notes::D5, 8},  {notes::C5, 8},
    {notes::REST, 8}, {notes::B4, 4},  {notes::C5, 8},  {notes::D5, 4},  {notes::E5, 4},
    {notes::REST, 8}, {notes::C5, 4},  {notes::A4, 8},  {notes::A4, 4}, {notes::REST, 4},
};
constexpr uint16_t TEMPO_BPM = 144;
constexpr uint16_t COUNT     = sizeof(NOTES) / sizeof(NOTES[0]);

}  // namespace tetris
