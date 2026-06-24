# Architecture

Software design for an 8-bit MCU: a deliberately small structure, the pure-vs-hardware split,
and pin ownership.

## MCU constraints (the ground rules)

ATmega328: 2 KB SRAM, 32 KB flash. The LED framebuffer alone is 140 × 3 = 420 B (~20% of SRAM).

- **No dynamic allocation** in the hot path — avoid `new`, `String`, `std::vector`,
  `std::function`, RTTI/`dynamic_cast`. Static allocation only.
- **Shallow inheritance.** A vtable + a few virtuals is fine; deep hierarchies and
  factories-that-`new` are not.
- Use design *principles* (separation of concerns, dependency inversion); skip the pattern zoo.

## Why repo-per-board

This is the **Nano** repo. The board is the project boundary, so `nano` is in the name. A
different chip (ESP32) is a *separate* repo that reuses these patterns, not this code. "One
codebase → many deploys" still applies, but scoped to **multiple Nano units** (a cooktop Nano, a
microwave Nano) via PlatformIO environments — not across chip families.

## Structure (deliberately small)

~200 lines in **two source files**. A class-per-file MVC tree was over-built for that size and was
dropped (preserved in git history, commit `feat(cooktop)`).

- **`include/Cooktop.h`** — all hardware-free logic, so it unit-tests off-device:
  - `HobModel` — per-hob heat level (0..255)
  - `adcToLevel()` / `emaStep()` — dimmer scaling + smoothing
  - `heatColor()` — level → HSV ramp (off → deep red → orange; full saturation, never yellow/white)
- **`src/main.cpp`** — the Arduino glue and the loop: pins, FastLED setup, and the
  `read inputs → update model → render rings → show` flow. (MVC survives as that flow, not as
  separate classes.)

The one seam worth keeping is **pure logic vs hardware**: everything testable lives in `Cooktop.h`
with no `Arduino.h`/FastLED, which is exactly why `pio test -e native` runs it on the host.

## Pin ownership

Pins and power sit at the top of `src/main.cpp` — the only hardware-specific spot — not scattered
through the logic. The authoritative pin table is in [hardware](../hardware/).

## Deferred: the plugin (Strategy) seam

An `IAppliance` interface (so an app layer could drive cooktop/microwave/… uniformly) was built and
then removed as premature — there is one appliance. Reintroduce it when a second is real (the
original is in git history). Multiple **Nano units** would ship from this repo via PlatformIO
environments; a different **chip** is a separate repo (see "Why repo-per-board").

## Patterns: use vs avoid

- **Use (lightly):** separation of pure logic from hardware, central constants, plain functions.
- **Avoid on Nano:** dynamic allocation, deep inheritance, `String`/STL containers/RTTI, and any
  abstraction without a second use case yet.
