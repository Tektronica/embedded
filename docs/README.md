# fw-nano-ledStripDimmer

A generic **N-channel dimmer → WS2812 LED-strip controller** for the Arduino Nano. Each channel reads
one dimmer (potentiometer) and drives one WS2812 LED strip through a level→color curve.

> **Naming:** `[codingType]-[targetPlatform]-[projectType]` → `fw` · `nano` · `ledStripDimmer`. Nano
> build; a different board is a *separate* repo. See [architecture](architecture/).

## Concept

- **N channels** (default `CHANNEL_COUNT` = 4): each is one dimmer input → one WS2812 LED strip output;
  the dimmer sets a smoothed 0–255 **level** (brightness).
- **Optional color switch** cycles a `Palette` (heat red→orange-red [default], green, blue, white,
  rainbow-per-strip); **optional mode switch** cycles a `Mode` (solid [default], blink, strobe, chase).
- Both switches use `INPUT_PULLUP`, so unwired they hold the defaults.
- All LED strips chain on one data pin as one logical buffer
  (`LED_TOTAL` = `CHANNEL_COUNT` × `LED_STRIP_LENGTH` = 140).

## Status

Firmware implemented, built, unit-tested (19/19), and running on a Nano (current build flashed).
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
