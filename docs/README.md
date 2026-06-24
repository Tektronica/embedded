# fw-nano-playKitchen

Firmware for an Arduino Nano–based toy play kitchen. The first appliance is a **cooktop**: four
WS2812 RGB LED rings act as dimmable "hot" hob burners, each controlled by its own physical
dimmer (potentiometer).

> **Naming:** `[codingType]-[targetPlatform]-[projectType]` → `fw` · `nano` · `playKitchen`. This
> is the Nano build; a different board is a *separate* repo (e.g. `fw-esp32-playKitchen`) unless it
> shares this codebase, in which case it's colocated here as another PlatformIO environment. See
> [architecture](architecture/) for why.

## Concept

- 4 × 35-LED WS2812 rings (140 LEDs total), data daisy-chained, power injected per ring.
- 1 data signal from the Nano drives all four rings as one logical strip.
- 4 dimmers wired to the Nano's analog inputs — one per hob — set each burner's "heat" level.
- Heat level maps to color/brightness (off → deep red → orange → bright yellow-white).

## Status

Firmware implemented, built, unit-tested (9/9), and flashed to a Nano. Not yet verified on wired
rings/dimmers — use the [Wokwi sim](simulation/) or a hardware bring-up.

## At a glance

| Area | Choice |
|---|---|
| MCU | Arduino Nano (ATmega328) |
| LEDs | 4 × WS2812 35-LED rings (140 total) |
| LED library | FastLED (HSV + built-in power limiting) |
| Toolchain | VSCode + PlatformIO (no Arduino IDE required) |
| Architecture | Two files — pure logic (`Cooktop.h`) + Arduino glue (`main.cpp`) |

## Docs (pillars)

- **[setup/](setup/)** — toolchain, building, flashing the Nano
- **[hardware/](hardware/)** — BOM, wiring, power budget & injection, pinout
- **[architecture/](architecture/)** — software design: structure, pure-vs-hardware split, pin ownership
- **[appliances/](appliances/)** — appliance specs + the (deferred) plugin contract
- **[troubleshooting/](troubleshooting/)** — environment/setup gotchas and their fixes
- **[simulation/](simulation/)** — run it in Wokwi (VS Code), no hardware

## Roadmap

1. Cooktop appliance: 4 hobs, dimmer-driven, heat-color mapping.
2. Extract the `IAppliance` plugin contract once a second appliance (e.g. microwave) is real.
3. Per-unit PlatformIO environments if multiple Nano-based appliances ship from this repo.
