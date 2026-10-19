# Transpiling a song into buzzer-song

This is a walkthrough of how a song gets from a source like
[robsoncouto/arduino-songs](https://github.com/robsoncouto/arduino-songs) into a working
`include/songs/<Song>.h` in this project — what the source format actually encodes, why our
format looks the way it does, and how `scripts/transpile-song.sh` gets you from one to the other
without hand-typing hundreds of notes. Tetris, Mario, Doom, and Nokia were all added this way; the
process is the same for the next one.

## 1. What the source format actually encodes

Every sketch in that repository follows the same shape. Strip away the boilerplate and three
things matter:

**A pitch table.** A block of `#define NOTE_C4 262` lines, one per semitone across roughly ten
octaves. This is a frequency in Hz — the physical thing a piezo buzzer needs to know to vibrate
at the right pitch. It's the same table, copy-pasted, at the top of every sketch in the repo
(including notes the song never actually uses — more on that below).

**A tempo.** One line, `int tempo = 144;` — beats per minute. Everything else derives from this.
The sketch computes `wholenote = (60000 * 4) / tempo`: 60,000 ms per minute, times 4 beats per
whole note, divided by tempo. That's the length of a whole note in milliseconds at this song's
speed.

**A melody array.** Alternating (pitch, duration-divider) pairs:

```cpp
NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4, ...
```

The divider is the denominator of a fraction of the whole note: `4` is a quarter note
(`wholenote / 4`), `8` an eighth, `16` a sixteenth, `2` a half, `1` the whole note itself. A
**negative** divider means *dotted* — that duration plus half again (`wholenote/4 * 1.5` for a
dotted quarter) — the same convention Western sheet music uses for a dot after a note. `REST`
(frequency 0) is silence with its own duration, not a special case.

That's the entire format. No note has a note-on/note-off delta the way MIDI does; duration is
implicit in the divider, and the whole sequence is meant to be played back start to end, once,
by a blocking `for` loop with a `delay()` call per note.

## 2. Why our format isn't a straight copy

We could store exactly that: pitch and a divider, and compute milliseconds from tempo at
playback time. That's actually exactly what we do — `song::Note{frequencyHz, durationDivider}`
mirrors the source almost token-for-token, and `song::durationMs(divider, tempoBpm)` in
`include/Song.h` does the same division the source's `for` loop does inline.

Two things had to change:

**Storage.** The AVR toolchain copies every ordinary `const` array from flash into RAM at boot,
whether or not the array is ever mutated — "const" only means the *program* won't write to it,
not that the compiler is free to leave it in flash. Tetris (97 notes), Mario (321), and
especially Doom (680) don't fit alongside everything else in the ATmega328P's 2 KB of SRAM as
plain arrays. `PROGMEM` tells the compiler to leave the array in flash and emit special
load-from-program-memory instructions (`pgm_read_word`, `pgm_read_byte`) everywhere it's read.
That's a real instruction-set distinction, not a hint — a plain pointer dereference into a
`PROGMEM` array reads garbage on real hardware. It's also why the PROGMEM-aware read loop
(`songplayer::toneStateForProgmem`) can't live in the same header as the pure duration math:
`avr/pgmspace.h` doesn't exist for the native/host build the unit tests run under, so anything
touching PROGMEM has to sit in a separate, hardware-coupled file.

**Organization.** The source repo is one sketch per song, each a full, disposable Arduino
project with its own copy-pasted playback loop. We want N songs sharing one playback engine, so
each song becomes *only* its data (a `NOTES[]` table, a `TEMPO_BPM`, a `COUNT`) in its own file
under `include/songs/`, and `include/SongPlayer.h` owns the one generic loop that plays any of
them. Adding a song is adding a file, never touching the engine.

## 3. The pitch-name problem

The source's `NOTE_GS5` (G-sharp, octave 5) becomes our `notes::Gs5`. Two small, mechanical
changes: the `NOTE_` prefix becomes our `notes::` namespace, and the sharp letter is
lowercased — `#` isn't legal in a C++ identifier, and this repo's convention (`Notes.h`) is a
lowercase `s` suffix rather than spelling out `Sharp`.

The one thing worth actually checking, not just transforming: does `Notes.h` already define
every pitch the song uses? Doom's melody reaches down into octave 2 (`E2`, `A2`, `As2`, `B2`) —
notes `Notes.h` didn't have, because every earlier song stayed in octaves 3–6. Rather than
pre-populating the full range "just in case," `Notes.h` grows one pitch at a time, only when a
real song needs it — see its header comment. Grep the transpiled output for `notes::[A-Za-z]*[0-9]`,
`sort -u`, and check each one is defined before you compile.

## 4. Doing it by hand doesn't scale

Tetris is 97 notes. Doom is 680. At that size, retyping `NOTE_E2, 8,` as `{notes::E2, 8},` by
eye, six hundred times, is exactly the kind of task where a single transposed digit produces a
melody that's subtly, audibly wrong and very hard to spot by reading the code — you'd have to
actually play it to notice. `scripts/transpile-song.sh` does the mechanical part; you still do
the two judgment calls that need a human (checking the pitch range, and picking a sensible file
name), but the 97-to-680 tokens in between never get hand-copied.

## 5. What the script actually does

Five stages, each addressing one specific wrinkle in the source format:

1. **Extract the array body.** `awk` prints everything between the `melody[] = {` line and the
   closing `};`, discarding the pitch table and the playback loop — neither is needed.

2. **Strip comments.** The source annotates measure numbers inline (`// 23`, `//repeats from 7`).
   Harmless in the source's own one-line-per-measure layout, but Doom's transcription (as you
   supplied it) put one token per line — a comment on a line by itself would otherwise swallow
   part of the next token once everything gets joined in the next step.

3. **Join into one stream.** `tr '\n' ' '` erases the source's original line breaks entirely.
   This matters because the same pair-matching regex has to work whether the source wrote eight
   notes per line (Tetris, Mario) or one token per line (the Doom transcription) — joining first
   means the line-layout question stops mattering.

4. **Rename tokens.** `NOTE_` → `notes::`, `([A-G])S([0-9])` → `\1s\2` (the sharp-lowering step —
   applied only inside a `notes::`-prefixed token, so it can't accidentally touch anything else),
   and bare `REST` → `notes::REST`.

5. **Pair up and re-wrap.** A pitch token followed by a signed integer becomes
   `{notes::X, N},`, one pair per output line, then regrouped eight-per-line to match the layout
   the existing `songs/*.h` files use.

## 6. Using it

```bash
arduino-nano/buzzer-song/scripts/transpile-song.sh path/to/song.ino > /tmp/song.pairs
```

The script also prints a note count to stderr. Cross-check that number against the source
file's own accounting — every one of these sketches computes
`notes = sizeof(melody) / sizeof(melody[0]) / 2` for its own playback loop, so the count is
already sitting right there in a comment or a variable; if the script's count doesn't match, stop
and find out why before trusting the output; a truncated `awk` extraction (a slightly different
`melody[]` declaration style) is the most likely cause.

Then, by hand:

1. Check every distinct pitch in the output exists in `Notes.h` (§3); add any that don't.
2. Paste the pairs into a new `include/songs/<Song>.h`, following `Doom.h` or `Nokia.h` as a
   template: the `PROGMEM` array, a `TEMPO_BPM` constant (straight from the source's `tempo`
   variable), a `COUNT` constant, and an attribution/license comment naming the original
   composer and this repo's non-commercial hobby-use framing.
3. Add one value to `song::Track` in `Song.h`, and one `case` in `songplayer::update()` in
   `SongPlayer.h`.
4. Build and test: `pio test -e native`, `pio run`, `pio check`. Then confirm the new song's
   `PROGMEM` table actually landed in flash, not RAM —
   `avr-nm --size-sort -C .pio/build/arduino-nano/firmware.elf | grep <song>` should show a size
   equal to `notes × 3 bytes` (this repo's `Note` packs to 3 bytes on AVR with no padding); if
   RAM usage jumped instead of flash, something upstream of `PROGMEM` isn't doing what it's
   supposed to.

That last check is the one step it's easy to skip and regret — a typo in the `PROGMEM` keyword
or the array declaration compiles fine and plays fine in Wokwi, and only shows up as a mysterious
RAM shortage on real hardware once several songs are loaded at once.
