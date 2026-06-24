# Architecture

Software design for an 8-bit MCU: a deliberately small structure, the pure-vs-hardware split, and
pin ownership.

## MCU constraints (the ground rules)

ATmega328: 2 KB SRAM, 32 KB flash. The LED framebuffer alone is `LED_TOTAL` × 3 = 420 B (~20% of SRAM).

- **No dynamic allocation** in the hot path — no `new`, `String`, `std::vector`, RTTI. Static only.
- Use design *principles* (separation of concerns); skip the pattern zoo.

## Structure (two files)

~200 lines. (An MVC class tree + an `IAppliance` plugin layer were tried and dropped as over-built —
preserved in git history.)

- **`include/LEDStripDimmer.h`** — all hardware-free logic, so it unit-tests off-device:
  - `Levels` — per-channel level (0..255)
  - `adcToLevel()` / `emaStep()` — dimmer scaling + smoothing
  - `levelColor()` — level → HSV curve (default: off → deep red → orange; swappable, no app meaning)
- **`src/main.cpp`** — Arduino glue + the loop: **read dimmer inputs → render LED strip outputs**.

The one seam worth keeping is **pure logic vs hardware**: everything testable lives in
`LEDStripDimmer.h` with no `Arduino.h`/FastLED, which is exactly why `pio test -e native` runs it on
the host.

> The header is `LEDStripDimmer.h`, not `Controller.h`, to avoid clashing with FastLED's
> `controller.h` on case-insensitive filesystems.

## Pin ownership

Pins and power sit at the top of `src/main.cpp` — the only hardware-specific spot — not scattered
through the logic. The authoritative pin table is in [hardware](../hardware/).

## Why repo-per-board

The board is the project boundary, so `nano` is in the repo name. A different chip (ESP32) is a
*separate* repo that reuses these patterns, not this code — multiple Nano units could still ship from
one repo via PlatformIO environments.

## Patterns: use vs avoid

- **Use (lightly):** the pure-logic-vs-hardware split, central constants, plain functions.
- **Avoid on Nano:** dynamic allocation, deep inheritance, `String`/STL containers/RTTI, and any
  abstraction without a second use case yet.
