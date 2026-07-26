# arduino-nano/buzzer-patterns

> Arduino Nano firmware: a passive buzzer and three momentary pushbuttons, each triggering a
> named alert/feedback tone pattern (KeyPress, Done, Error).

This project lives at `arduino-nano/buzzer-patterns/` in the `embedded` monorepo. It's the
standalone counterpart to `toy-microwave`'s `Buzzer.h` — that project consumes these patterns for
one specific application (key-press feedback and a done chime on a cook timer); this project is
where the technique itself lives and gets exercised on its own, the same relationship `keypad` has
to `toy-microwave`'s matrix-scanning technique. See also `arduino-nano/buzzer-song` (looping
melodies) and `arduino-nano/buzzer-simple` (the naive blocking version) — together the three
projects cover feedback tones, music, and the naive baseline.

## What it does

Three buttons, three one-shot **patterns**:

| Button | Pattern | Sound |
|---|---|---|
| KeyPress | `Pattern::KeyPress` | a single short 2kHz blip |
| Done | `Pattern::Done` | four 2.5kHz beeps with gaps between them |
| Error | `Pattern::Error` | one longer, low 300Hz tone |

Pressing a button starts its pattern; the buzzer plays it to completion (or restarts it if pressed
again) and then falls silent until the next press. Non-blocking throughout — no `delay()` calls.

## Design

- **`include/Buzzer.h`** — hardware-free (unit-tested via `pio test -e native`): the `Pattern`
  enum (`None`/`KeyPress`/`Done`/`Error`), `ToneState{on, frequencyHz}`, `toneStateFor(pattern,
  elapsedMs)`, and `isFinished(pattern, elapsedMs)` — the same pattern/`ToneState` shape as
  `toy-microwave`'s and `game-dino-run`'s `Buzzer.h`, verbatim except dropping `Hum` (that pattern
  is tied to a microwave's continuous Running state, not a one-shot button press).
- **`include/Button.h`** — a debounced push-button edge detector (unit-tested), duplicated from
  `stepper`'s `Button.h` per this repo's convention of small utilities living standalone in each
  project rather than a shared library.
- **`src/main.cpp`** — pins, three `Button` instances each starting their own `Pattern` on a fresh
  press, and the loop mapping `buzzer::toneStateFor()` to `tone()`/`noTone()`, resetting to
  `Pattern::None` once `isFinished()` says the active pattern's sequence has completed.
  `DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot-format trace of the active pattern and
  tone state.

Kept deliberately small — one pure pattern/timing header, one debounced button, one thin
`main.cpp` with no state machine beyond "which pattern, since when."

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/buzzer-patterns
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Passive buzzer | D9 | driven via `tone()`/`noTone()`, other leg to GND |
| KeyPress button | D2 | `INPUT_PULLUP`, other leg to GND |
| Done button | D3 | `INPUT_PULLUP`, other leg to GND |
| Error button | D4 | `INPUT_PULLUP`, other leg to GND |

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (`wokwi-buzzer` + three pushbuttons). Build
(`pio run`), then **F1 → "Wokwi: Start Simulator"** and press KEYPRESS/DONE/ERROR to hear each
pattern.

## Status

Built and tested (native unit tests for `Buzzer.h`/`Button.h`); not yet verified against real
hardware.
