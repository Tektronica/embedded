# arduino-nano/seven-segment

> Arduino Nano firmware: a standalone 4-digit 7-segment display driver. One button cycles the
> same on-screen value through Static, Flashing, and Rolling presentation — proving the mode
> never depends on what's being shown.

This project lives at `arduino-nano/seven-segment/` in the `embedded` monorepo. It's the
standalone counterpart to `toy-microwave`'s display — that project consumes a 7-segment display
for one specific value (MM:SS/HH:MM); this project is where the general-purpose display
technique itself lives, the same relationship `keypad` has to `toy-microwave`'s matrix-scanning
and `buzzer-patterns` has to its `Buzzer.h`.

## What it does

**Content** (what to show) and **Mode** (how to show it) are independent — a developer sets a
number or a label without knowing which mode is active, and can change the mode without touching
the value:

- `numberContent(n)` — a number, 0-9999.
- `labelContent(text, length)` — a short string, e.g. `"DONE"` or a longer phrase to scroll.

## Modes

| Mode | Behavior | Applies to |
|---|---|---|
| **Static** | Shows the content as-is, left-justified, blank-padded to 4 characters. | Number, Label |
| **Flashing** | Alternates between Static's output and fully blank, once per `periodFrames`. | Number, Label |
| **Rolling** | Scrolls a 4-character window across the label, one character every `periodFrames` frames, with a 4-character blank gap before it repeats. A `Number` is always ≤4 digits already, so Rolling falls back to Static for it rather than doing nothing meaningful. | Label only (Number falls back to Static) |

`periodFrames` means the on/off cycle length under Flashing, and the frames-per-scroll-step under
Rolling — the caller (`main.cpp`) owns the actual timing (how often a "frame" ticks, and how many
frames per mode-specific unit), matching how `toy-microwave`'s `BLINK_PERIOD_MS` lives in that
project's `main.cpp`, not in `SevenSegment.h`.

**Colon blink is not a fourth mode.** A blinking colon is a decorative segment on one specific
digit (e.g. between MM and SS), not a value-presentation strategy — a real clock shows a static
*or* rolling value with an independently blinking colon, so making it a `Mode` value would
wrongly make it mutually exclusive with Flashing/Rolling. Instead, `withColon(segments,
colonDigitIndex, colonOn)` ORs the colon segment into an already-`render()`'d `Segments`,
composing with any mode: `withColon(render(content, mode, frame, periodFrames), 1,
blinkOn(frame, colonPeriodFrames))`. `colonDigitIndex` and which bit is actually wired to the
colon vary by board — confirm against your display, same caveat `toy-microwave`'s
`SevenSegment.h` notes for its own colon wiring.

The demo firmware cycles Static → Flashing → Rolling on each button press, restarting whichever
mode's animation from frame 0 — and deliberately never changes *what's* shown when it does. Every
mode displays the same live counter, formatted as a 12-character label (`"NNNN    NNNN"`, the
counter twice with a gap): Static and Flashing only ever render the first 4 characters, so they
show the counter plainly; Rolling scrolls the full label, so the same counter value slides across
the window and reappears. Same content the whole time — pressing the button changes only how it's
drawn, whether the underlying value would fit in 4 characters (like a clock's `"11:23"`) or not.

## Design principles

- **Single Responsibility**: `segmentsForChar()` (character → segment bits), `rollOffset()`
  (scroll timing), `blinkOn()` (blink timing), and `render()` (composition) each do exactly one
  job. `Content` (value) and `Mode` (presentation) are separate types for the same reason —
  changing one was never supposed to require touching the other.
- **Open/Closed**: a new `Mode` is one enum value plus one `case` in `render()` — `segmentsForChar()`
  and `rollOffset()` never change. A new displayable character is one line in `segmentsForChar()`'s
  switch — nothing else changes. The two extension axes (presentation repertoire vs. character
  vocabulary) don't interfere with each other.
- **Liskov/Interface Segregation**: not directly applicable — there's no inheritance or interface
  here, deliberately. See "why not a class hierarchy" below.
- **Dependency Inversion**: `render()` depends only on `Content`/`Mode` and the pure helpers, never
  on any hardware type. `main.cpp` (the hardware-specific detail) depends on `render()`'s output,
  never the reverse — the same hardware-free/hardware-coupled direction every project in this repo
  follows.
- **DRY**: one `segmentsForChar()` table serves digits, letters, Static, Flashing, and Rolling —
  there's no separate digit-encoding path duplicating part of what TM1637Display's own
  `encodeDigit()` does. `blinkOn()` is copied from `toy-microwave`'s `SevenSegment.h` verbatim
  rather than reimplemented (per this repo's small-utility-duplication convention — see
  `Button.h`), not redefined with different behavior.
- **Why not a Strategy-pattern class hierarchy**: a `Mode` enum with one `switch` inside `render()`
  is a Strategy pattern in intent (pick a presentation algorithm independent of the data it's
  applied to) without the GoF pattern's literal class-per-strategy machinery. The mode set is
  small and fixed at compile time on an 8-bit target with no heap — virtual dispatch would cost
  flash and indirection for flexibility (runtime-pluggable strategies) nothing here actually needs.
  Same reasoning `led-dimmer-ws2812`'s architecture doc gives for its own `pixelColor()` primitive.

## Design

- **`include/SevenSegment.h`** — hardware-free (unit-tested via `pio test -e native`):
  `segmentsForChar()`, `Content`/`ContentType`/`numberContent()`/`labelContent()`, `Mode`,
  `blinkOn()`, `rollOffset()`, and `render()` (the single render primitive tying all of the above
  together into the four segment bytes to show right now).
- **`include/Button.h`** — a debounced push-button edge detector (unit-tested), duplicated from
  `stepper`'s `Button.h` per this repo's convention of small utilities living standalone in each
  project rather than a shared library.
- **`src/main.cpp`** — TM1637 wiring (same CLK/DIO setup as `toy-microwave`), `currentMode`
  cycling `sevenseg::Mode::Static`/`Flashing`/`Rolling` on each button press, a `frame` counter
  ticked every 50ms, and the loop mapping `sevenseg::render()`'s output straight to
  `TM1637Display::setSegments()`. The counter and its label are computed once per loop and
  handed to `render()` unchanged regardless of `currentMode` — nothing about the content path
  branches on which mode is active.

One real gotcha worth flagging: `TM1637Display.h` defines `SEG_A`..`SEG_DP` as C preprocessor
`#define` macros, not namespaced constants. A same-named `constexpr` in `SevenSegment.h` compiles
fine on its own (nothing pulls in `TM1637Display.h` there) but fails the moment `main.cpp`
includes both headers together, since the macro silently rewrites the `constexpr` declaration
before the compiler ever sees it. `SevenSegment.h`'s bit constants are named `SEGBIT_A`..
`SEGBIT_DP` specifically to avoid this — caught by `pio check`/`pio run` failing even though
`pio test -e native` (which never includes `TM1637Display.h`) passed clean.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/seven-segment
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Display CLK | A0 | same TM1637 wiring as `toy-microwave` |
| Display DIO | A1 | |
| Mode button | D2 | `INPUT_PULLUP`, other leg to GND — cycles Static/Flashing/Rolling |

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (`wokwi-tm1637-7segment` + a pushbutton). Build
(`pio run`), then **F1 → "Wokwi: Start Simulator"** and press the button to cycle through the
three modes.

## Status

Built and tested (16/16 native unit tests across `SevenSegment.h`/`Button.h`); not yet verified
against real hardware.
