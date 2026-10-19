# Seven-Segment Display

A general-purpose driver for a 4-digit 7-segment display on a TM1637 board. One button cycles a
live counter through three presentation modes, Static, Flashing, and Rolling, without changing the
value shown. The example demonstrates that what to show (Content) and how to show it (Mode) are
independent.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano (`nanoatmega328`), ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/SevenSeg`, `lib/arduino/Debounce` |
| Other libraries | [avishorp/TM1637](https://github.com/avishorp/TM1637) (from GitHub) |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests (`env:native`) for `include/SevenSegment.h`.
- Wokwi simulator (`diagram.json`, `wokwi.toml`).

## Getting started

If `pio` is not on your shell `PATH` (VS Code extension only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

Run from the repo root:

1. Build: `pio run -d examples/arduino-nano/seven-segment`
2. Build and flash the Nano: `pio run -d examples/arduino-nano/seven-segment -t upload`
3. Run the native tests: `pio test -d examples/arduino-nano/seven-segment -e native`

## Hardware

- Arduino Nano
- 4-digit 7-segment display board with a TM1637 driver
- Momentary pushbutton

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Display CLK | A0 | |
| Display DIO | A1 | |
| Mode button | D2 | `INPUT_PULLUP`, other leg to GND |

## How it works

### Content

- `numberContent(n)`: a number, 0 to 9999. Larger values clamp to 9999.
- `labelContent(text, length)`: a string, such as `"HELP"` or a longer phrase to scroll.

### Modes

| Mode | Behavior | Applies to |
| --- | --- | --- |
| Static | Shows the content left-justified, blank-padded to 4 characters. | Number, Label |
| Flashing | Alternates between the Static output and blank, on a `periodFrames` cycle. | Number, Label |
| Rolling | Scrolls a 4-character window across the label, one step every `periodFrames` frames, with a 4-character blank gap before it repeats. | Label. A Number falls back to Static. |

The caller owns timing. `src/main.cpp` ticks one frame every 50 ms. Flashing uses a 20-frame
period (~1 s), and Rolling steps every 6 frames (~300 ms).

### Colon

A blinking colon is not a fourth mode, since a clock shows a static or rolling value with an
independently blinking colon. `withColon(segments, colonDigitIndex, colonOn)` ORs
`sevenseg::SEGBIT_DP` into one digit of an already rendered `Segments`, so it composes with any
mode: `withColon(render(content, mode, frame, periodFrames), 1, blinkOn(frame, colonPeriodFrames))`.
The colon digit and bit vary by board. Confirm them against your display. The demo does not use
`withColon()`.

### Demo firmware

Each button press advances Static, Flashing, Rolling, and back to Static, and restarts the frame
counter at 0. The counter increments once per second, wraps at 10000, and is formatted as a
12-character label, `"NNNN    NNNN"`. Static and Flashing render the first 4 characters, so they
show the counter. Rolling scrolls the full label, so the same value slides across and reappears.
The content path never branches on the active mode.

### Firmware layout

- `include/SevenSegment.h` (namespace `segdisplay`): hardware-free. `Content`, `ContentType`,
  `numberContent()`, `labelContent()`, `Mode`, `rollOffset()`, `withColon()`, and `render()`,
  which returns the four segment bytes to show now.
- `lib/arduino/SevenSeg`: the font (`encodeChar()`), `blinkOn()`, and the `SEGBIT_*` constants.
- `lib/arduino/Debounce`: `debounce::Button`, the debounced press edge detector.
- `src/main.cpp`: TM1637 setup, mode cycling, the frame and counter timers, and a call to
  `TM1637Display::setSegments()` with `render()`'s output.

A `Mode` enum with one `switch` in `render()` replaces a Strategy class hierarchy. The mode set is
fixed at compile time on an 8-bit target with no heap, so virtual dispatch would cost flash for
flexibility nothing here uses. A new mode is one enum value plus one `case` in `render()`. A new
character is one line in `sevenseg::encodeChar()`.

`TM1637Display.h` defines `SEG_A` to `SEG_DP` as preprocessor macros. A `constexpr` with the same
name compiles alone but breaks once `main.cpp` includes both headers. `lib/arduino/SevenSeg` names
its constants `SEGBIT_A` to `SEGBIT_DP` for this reason. The native tests never include
`TM1637Display.h`, so only `pio run` catches a collision.

## Simulate

The Wokwi diagram has a `wokwi-tm1637-7segment` and a pushbutton. Build, then run
**F1 > Wokwi: Start Simulator** in VS Code and press the button to cycle the modes.

## Known issues

- Auto-reset is unreliable on this CH340 Nano. Tap RESET as the upload starts.

## Status

Built, with 12 native unit tests for `SevenSegment.h` plus the `SevenSeg` and `Debounce` library
tests. Not yet verified on real hardware.
