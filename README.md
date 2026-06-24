# fw-nano-playKitchen

> Arduino Nano firmware for a toy play kitchen. First appliance: a **cooktop** with four WS2812 RGB
> LED rings as dimmable "hot" hob burners, each driven by its own physical dimmer.

Repo name follows `[codingType]-[targetPlatform]-[projectType]`: `fw` · `nano` · `playKitchen`.
This is the Nano build. A different board is a *separate* repo (e.g. `fw-esp32-playKitchen`) unless
it shares this codebase — then it's colocated here as another PlatformIO environment.

## Features

- 4 hob burners — 35-LED WS2812 rings, data daisy-chained into one 140-LED strip.
- One dimmer (potentiometer) per hob; position maps to a heat ramp
  (off → deep red → bright orange, like a glowing burner element).
- Single data line drives all four rings; power injected per ring.
- FastLED with built-in current limiting to stay inside the power budget.
- MVC + a Strategy-style `IAppliance` plugin so new appliances drop in cleanly.

## Hardware

- Arduino Nano (ATmega328, 5 V)
- 4 × WS2812 5050 RGB LED rings, 35 LEDs each
- 4 × potentiometers (dimmers)
- 5 V PSU (sized for the LED load), data-line resistor, bulk capacitor

Full BOM, power budget, wiring rules, and pinout: **[docs/hardware](docs/hardware/)**.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.

```bash
git clone <repo-url>
cd fw-nano-playKitchen
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
```

Bootloader/CH340/port gotchas and the `platformio.ini` starting point: **[docs/setup](docs/setup/)**.

**Tests** — pure logic off-device: `pio test -e native`.

## Repository structure

```
fw-nano-playKitchen/
├── platformio.ini      # env:nano (build/flash) + env:native (host unit tests)
├── include/
│   └── Cooktop.h       # hardware-free logic: hob state, ADC→level, heat-color ramp (unit-tested)
├── src/
│   └── main.cpp        # Arduino glue: pins, FastLED, loop = read → update → render
├── test/
│   └── test_cooktop/   # native Unity tests for Cooktop.h
├── docs/               # project documentation (see below)
└── README.md
```

## Documentation

| Pillar | Contents |
|---|---|
| [docs/](docs/) | Project overview |
| [docs/setup](docs/setup/) | Toolchain, building, flashing |
| [docs/hardware](docs/hardware/) | BOM, power budget & injection, wiring, pinout |
| [docs/architecture](docs/architecture/) | Structure, pure-vs-hardware split, pin ownership |
| [docs/appliances](docs/appliances/) | Appliance specs + the (deferred) plugin contract |
| [docs/simulation](docs/simulation/) | Run in Wokwi (VS Code), no hardware |
| [docs/troubleshooting](docs/troubleshooting/) | Environment/setup gotchas + fixes |

## Status

Early scaffolding — design decided, firmware not yet written.

## Roadmap

1. Cooktop appliance: 4 hobs, dimmer-driven, heat-color mapping.
2. Extract the `IAppliance` plugin contract once a second appliance (e.g. microwave) is real.
3. Per-unit PlatformIO environments if multiple Nano-based appliances ship from this repo.
