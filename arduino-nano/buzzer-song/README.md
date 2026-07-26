# arduino-nano/buzzer-song

> Arduino Nano firmware: a passive buzzer driven by a momentary pushbutton. Each press cycles to
> the next track — a scale, and four songs borrowed from a well-known Arduino tune collection.

This project lives at `arduino-nano/buzzer-song/` in the `embedded` monorepo. See
`arduino-nano/buzzer-simple/` for the naive, blocking `tone()`+`delay()` counterpart this project
is contrasted against.

## What it does

A passive buzzer loops one of five **tracks**, cycled in order by one **momentary pushbutton**
(same debounced-edge-toggle pattern as `stepper`'s run/stop and direction buttons):

| Track | Source | Notes |
|---|---|---|
| **Scale** | original | do-re-mi-fa-so-la-ti-do, repeating |
| **Tetris** | traditional ("Korobeiniki," public domain) | full theme, 97 notes |
| **Mario** | Koji Kondo / Nintendo, 1985 | "Overworld" theme, 321 notes — personal, non-commercial hobby use only; copyright remains with Nintendo |
| **Doom** | Bobby Prince / id Software, 1993 | "At Doom's Gate" (E1M1), 680 notes — personal, non-commercial hobby use only; copyright remains with id Software |
| **Nokia** | traditional (public domain), popularized by Nokia | the default ringtone excerpt, 13 notes |

Tetris, Mario, Doom, and Nokia were transcribed from
[robsoncouto/arduino-songs](https://github.com/robsoncouto/arduino-songs) (no license stated
there).

## Design

- **`include/Notes.h`** — hardware-free equal-temperament note frequency constants: octaves 3-6
  fully populated (the practical range for a small piezo buzzer), plus a handful of octave-2
  pitches added on demand as a song actually needs them (currently Doom's bass line) rather than
  the full octave. See the reference chart below for the full C0-B8 range.
- **`include/Song.h`** — hardware-free (unit-tested via `pio test -e native`): `Note{frequencyHz,
  durationDivider}` using the classic tracker/ringtone convention (4=quarter, 8=eighth,
  negative=dotted), `durationMs(divider, tempoBpm)` to convert that to milliseconds, the `Track`
  enum (`Scale`/`Tetris`/`Mario`/`Doom`/`Nokia`) plus `next()` to cycle it, and `toneStateFor(notes,
  count, tempoBpm, elapsedMs)` — the pure playback algorithm: loop the sequence, and sound each
  note for `NOTE_ON_PERCENT` (90%) of its duration so repeated same-pitch notes are heard
  separately. `Scale`'s own small note table lives here too. Same `ToneState` shape as
  `toy-microwave`'s and `game-dino-run`'s `Buzzer.h`.
- **`include/songs/Tetris.h`**, **`Mario.h`**, **`Doom.h`**, **`Nokia.h`** — one song per file,
  grouped under `songs/` so the catalog doesn't blend into the engine files as it grows. Just its
  `PROGMEM` note table (`NOTES`), `TEMPO_BPM`, and `COUNT`, plus its own attribution/license
  comment. `PROGMEM` since Tetris (97 notes), Mario (321), and especially Doom (680) don't fit the
  ATmega328P's 2KB SRAM as plain arrays (AVR copies ordinary `const` globals from flash into RAM
  at boot regardless of constness — only `PROGMEM` keeps them flash-resident, read back via
  `pgm_read_word`/`pgm_read_byte`); Nokia (13 notes) is small enough to not need it but is kept
  PROGMEM anyway for one consistent storage path across the whole borrowed-song catalog.
- **`include/SongPlayer.h`** — hardware-coupled (not unit-tested; needs `avr/pgmspace.h`), same
  shape as `stepper`'s `Stepper.h`/`robot-buoy`'s `Radio.h`: the generic PROGMEM playback engine
  (`toneStateForProgmem()`), reusing `Song.h`'s `durationMs()` for the actual timing math so every
  storage path agrees on how long a note lasts. `songplayer::update(currentTrack, elapsedMs)` is
  the single production entry point `main.cpp` calls, regardless of which track (or storage
  backing) is active. Scale stays RAM-resident since it's tiny, walked directly by
  `song::toneStateFor()`.
- **`include/Button.h`** — a debounced push-button edge detector (unit-tested), duplicated from
  `stepper`'s `Button.h` per this repo's convention of small utilities living standalone in each
  project rather than a shared library.
- **`src/main.cpp`** — pins, one `Button` instance advancing `currentTrack` via `song::next()` on
  each fresh press, and the loop mapping `songplayer::update()` to `tone()`/`noTone()`.
  `DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot-format trace of the active track and
  tone state.

Adding another song means a new header under `songs/` (its `NOTES`/`TEMPO_BPM`/`COUNT`) plus one
`Track` enum value and one `switch` case in `SongPlayer.h` — the engine itself (`toneStateFor()`/
`toneStateForProgmem()`) never changes, and no existing song's file is touched. This is the same
shape a hardware sound driver from the 8-bit console era would use: one generic sequencer reading
a compact note table out of ROM, rather than bespoke playback code per song (the pattern the
upstream `arduino-songs` sketches actually use, one copy-pasted playback loop per `.ino` file).

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/buzzer-song
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Passive buzzer | D9 | driven via `tone()`/`noTone()`, other leg to GND |
| Track button | D2 | `INPUT_PULLUP`, other leg to GND — cycles Scale/Tetris/Mario/Doom/Nokia on each fresh press |

## Note frequency reference

Equal-temperament note frequencies across the full audible octave range. `Notes.h` only defines
octaves 3-6 (the practical range for a small piezo buzzer); this is the full chart for reference.

| Note | Octave 0 | Octave 1 | Octave 2 | Octave 3 | Octave 4 | Octave 5 | Octave 6 | Octave 7 | Octave 8 |
|---|---|---|---|---|---|---|---|---|---|
| C | 16.35 Hz | 32.70 Hz | 65.41 Hz | 130.81 Hz | 261.63 Hz | 523.25 Hz | 1046.50 Hz | 2093.00 Hz | 4186.01 Hz |
| C#/Db | 17.32 Hz | 34.65 Hz | 69.30 Hz | 138.59 Hz | 277.18 Hz | 554.37 Hz | 1108.73 Hz | 2217.46 Hz | 4434.92 Hz |
| D | 18.35 Hz | 36.71 Hz | 73.42 Hz | 146.83 Hz | 293.66 Hz | 587.33 Hz | 1174.66 Hz | 2349.32 Hz | 4698.63 Hz |
| D#/Eb | 19.45 Hz | 38.89 Hz | 77.78 Hz | 155.56 Hz | 311.13 Hz | 622.25 Hz | 1244.51 Hz | 2489.02 Hz | 4978.03 Hz |
| E | 20.60 Hz | 41.20 Hz | 82.41 Hz | 164.81 Hz | 329.63 Hz | 659.25 Hz | 1318.51 Hz | 2637.02 Hz | 5274.04 Hz |
| F | 21.83 Hz | 43.65 Hz | 87.31 Hz | 174.61 Hz | 349.23 Hz | 698.46 Hz | 1396.91 Hz | 2793.83 Hz | 5587.65 Hz |
| F#/Gb | 23.12 Hz | 46.25 Hz | 92.50 Hz | 185 Hz | 369.99 Hz | 739.99 Hz | 1479.98 Hz | 2959.96 Hz | 5919.91 Hz |
| G | 24.50 Hz | 49 Hz | 98 Hz | 196 Hz | 392 Hz | 783.99 Hz | 1567.98 Hz | 3135.96 Hz | 6271.93 Hz |
| G#/Ab | 25.96 Hz | 51.91 Hz | 103.83 Hz | 207.65 Hz | 415.30 Hz | 830.61 Hz | 1661.22 Hz | 3322.44 Hz | 6644.88 Hz |
| A | 27.50 Hz | 55 Hz | 110 Hz | 220 Hz | 440 Hz | 880 Hz | 1760 Hz | 3520 Hz | 7040 Hz |
| A#/Bb | 29.14 Hz | 58.27 Hz | 116.54 Hz | 233.08 Hz | 466.16 Hz | 932.33 Hz | 1864.66 Hz | 3729.31 Hz | 7458.62 Hz |
| B | 30.87 Hz | 61.74 Hz | 123.47 Hz | 246.94 Hz | 493.88 Hz | 987.77 Hz | 1975.53 Hz | 3951.07 Hz | 7902.13 Hz |

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (`wokwi-buzzer` + a pushbutton). Build (`pio run`),
then **F1 → "Wokwi: Start Simulator"** and press the button to cycle through Scale, Tetris, Mario,
Doom, and Nokia.

## Status

Built and tested (native unit tests for `Song.h`/`Button.h`); not yet verified against real
hardware.
