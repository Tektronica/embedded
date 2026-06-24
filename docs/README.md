# fw-nano-ledStripDimmer

A generic **N-channel dimmer → WS2812 LED-strip controller** for the Arduino Nano. Each channel reads
one dimmer (potentiometer) and drives one WS2812 LED strip through a level→color curve.

> **Naming:** `[codingType]-[targetPlatform]-[projectType]` → `fw` · `nano` · `ledStripDimmer`. Nano
> build; a different board is a *separate* repo. See [architecture](architecture/).

## Concept

- **N channels** (default `CHANNEL_COUNT` = 4): each is one dimmer input → one WS2812 LED strip output.
- Dimmer position → smoothed 0–255 **level** → **color** via `levelColor` (a swappable curve).
- All LED strips are data-chained on one pin and addressed as one logical buffer
  (`LED_TOTAL` = `CHANNEL_COUNT` × `LED_STRIP_LENGTH` = 140).
- The default curve ramps off → deep red → orange-red; it carries no application meaning.

## Status

Firmware implemented, built, unit-tested (10/10), and running on a Nano (current build flashed).
Behavior verified in the Wokwi simulator; physical LED strips and dimmers not yet wired.

## At a glance

| Area | Choice |
|---|---|
| MCU | Arduino Nano (ATmega328) |
| Outputs | N × WS2812 LED strips (default 4 × 35 = 140 LEDs) |
| Inputs | N × dimmers (potentiometers, A0–A3) |
| LED library | FastLED (HSV + current cap) |
| Toolchain | VSCode + PlatformIO (no Arduino IDE required) |
| Architecture | Two files — pure logic (`LEDStripDimmer.h`) + Arduino glue (`main.cpp`) |

## Docs (pillars)

- **[setup/](setup/)** — toolchain, building, flashing the Nano
- **[hardware/](hardware/)** — BOM, wiring, power budget & injection, pinout
- **[architecture/](architecture/)** — software design: structure, pure-vs-hardware split, pin ownership
- **[simulation/](simulation/)** — run it in Wokwi (VS Code), no hardware
- **[troubleshooting/](troubleshooting/)** — environment/setup gotchas and their fixes
- **[application/](application/)** — example use: a toy-kitchen cooktop
